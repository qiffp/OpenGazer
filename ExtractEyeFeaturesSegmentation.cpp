
#include "ExtractEyeFeaturesSegmentation.h"
#include "EyeExtractor.h"
#include "Application.h"
#include <opencv/highgui.h>
#include <math.h>
#include <fstream>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64
#define SmoothBlockSize 29

#define BlockSegmentation 40

#define GRAY_LEVEL 127

#define SegmentationVideoFlag 0


const int ExtractEyeFeaturesSegmentation::eyeDX = 32;
const int ExtractEyeFeaturesSegmentation::eyeDY = 16;
const cv::Size ExtractEyeFeaturesSegmentation::eyeSize = cv::Size(eyeDX * 2, eyeDY * 2);


static std::vector<int> vector_static_horizontal(HORIZONTAL_BIN_SIZE, 0);
static std::vector<int> vector_static_vertical(VERTICAL_BIN_SIZE, 0);

ExtractEyeFeaturesSegmentation::ExtractEyeFeaturesSegmentation()
{
	elipse_gray = cv::imread("./elipse.jpg", CV_LOAD_IMAGE_GRAYSCALE);

	sizeImageDisk = 5;
	for (int j=0; j<VECTOR_SIZE; j++) {
		irisTemplateDisk[j] = constructTemplateDisk(sizeImageDisk+j);

		Gaussian2D[j].create(cv::Size(EyeExtractor::eyeSize.width - irisTemplateDisk[j].size().width + 1, EyeExtractor::eyeSize.height - irisTemplateDisk[j].size().height + 1), CV_32FC1);

		/* Gaussian2D[j] = TODO ONUR UNCOMMENTED, NOT NECESSARY */
		CreateTemplateGausian2D(Gaussian2D[j]);
	}

	this->histogram_horizontal.create(cv::Size(EyeExtractor::eyeSize), CV_8UC3);
	this->histogram_vertical.create(cv::Size(EyeExtractor::eyeSize.height, EyeExtractor::eyeSize.width), CV_8UC3);
	this->histogram_horizontal_left.create(cv::Size(EyeExtractor::eyeSize), CV_8UC3);
	this->histogram_vertical_left.create(cv::Size(EyeExtractor::eyeSize.height, EyeExtractor::eyeSize.width), CV_8UC3);
}

void ExtractEyeFeaturesSegmentation::process() {

	if (Application::Components::pointTracker->isTrackingSuccessful()) {

    	processToExtractFeatures();
    }
}



void ExtractEyeFeaturesSegmentation::processToExtractFeatures() {

	for(int side = 0; side < 2; side++) {
		cv::Mat finalIris;

		cv::Mat *eyeGrey 	= (side == 0) ? Application::Components::eyeExtractor->eyeGrey.get() : Application::Components::eyeExtractor->eyeGreyLeft.get();
		cv::Mat *eyeImage 	= (side == 0) ? Application::Components::eyeExtractor->eyeImage.get() : Application::Components::eyeExtractor->eyeImageLeft.get();
		std::vector<int> *vector_horizontal 	= (side == 0) ? &this->vector_horizontal 	: &this->vector_horizontal_left;
		std::vector<int> *vector_vertical 		= (side == 0) ? &this->vector_vertical 		: &this->vector_vertical_left;
		cv::Mat *histogram_horizontal 			= (side == 0) ? &this->histogram_horizontal : &this->histogram_horizontal_left;
		cv::Mat *histogram_vertical 			= (side == 0) ? &this->histogram_vertical 	: &this->histogram_vertical_left;
		std::vector<std::vector<int> > *histPositionSegmentedPixels = (side == 0) ? &this->histPositionSegmentedPixels : &this->histPositionSegmentedPixels_left;

		cv::Mat Temporary_elipsed(eyeGrey->size(), CV_8UC1);
		cv::rectangle(Temporary_elipsed, cv::Point(0,0), cv::Point(Temporary_elipsed.size().width,Temporary_elipsed.size().height), cv::Scalar(100,100,100), -1, 8);

		/*if (histogramFlag == 1) {	// ¿ ESTO ES NECESARIO ?

			eyeGrey->copyTo(Temporary_elipsed, elipse_gray);

		} else {

			eyeGrey->copyTo(Temporary_elipsed);
		}*/

		eyeGrey->copyTo(Temporary_elipsed);

	    static double *bestval;
	    static cv::Point *bestloc;
		
		int j;

		int comparison_method = CV_TM_CCOEFF_NORMED;

		for (j=0; j<VECTOR_SIZE; j++){


			Matches[j].create(cv::Size(Temporary_elipsed.size().width - irisTemplateDisk[j].size().width + 1, Temporary_elipsed.size().height - irisTemplateDisk[j].size().height + 1), CV_32FC1);
			
			MatchesSmoothed[j].create(cv::Size(Matches[j].size().width - (SmoothBlockSize - 1)/2, Matches[j].size().height - (SmoothBlockSize - 1)/2), CV_32FC1);

			matchTemplate(Temporary_elipsed, irisTemplateDisk[j], Matches[j], comparison_method);

			multiply(Matches[j], Gaussian2D[j], Matches[j]);

			GaussianBlur(Matches[j], Matches[j], cv::Size(SmoothBlockSize, SmoothBlockSize), 0, 0, CV_BLUR_NO_SCALE );

			Matches[j](cv::Rect((SmoothBlockSize - 1)/2, 
		    							(SmoothBlockSize - 1)/2, 
		    							Matches[j].size().width - (SmoothBlockSize - 1)/2, 
		    							Matches[j].size().height - (SmoothBlockSize - 1)/2)).copyTo(MatchesSmoothed[j]);

			minMaxLoc(MatchesSmoothed[j], &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);

		}

		double maxProbability = (double) MaxVal[0];

		double tmp;

		int i = 0;
		for (j=1; j<VECTOR_SIZE; j++){

			tmp = (double) MaxVal[j] * (1+j*j/300.0);

			if (tmp > maxProbability){

				maxProbability = tmp;
				i = j;
			}
		}

		int x1 = fmax(0, MaxLoc[i].x + (SmoothBlockSize - 1)/2 - (BlockSegmentation/2));
		int y1 = fmax(0, MaxLoc[i].y + (SmoothBlockSize - 1)/2 - (BlockSegmentation/2));
		int x2 = fmin(BlockSegmentation, Temporary_elipsed.size().width - MaxLoc[i].x + (SmoothBlockSize - 1)/2 - (BlockSegmentation/2));
		int y2 = fmin(BlockSegmentation, Temporary_elipsed.size().height - MaxLoc[i].y + (SmoothBlockSize - 1)/2 - (BlockSegmentation/2));


		if (SegmentationVideoFlag == 1) {

			cv::Mat *imageSegmentationVideo = (side == 0) ? Application::Components::eyeExtractorSegmentationGroundTruth->eyeImage.get() : Application::Components::eyeExtractorSegmentationGroundTruth->eyeImageLeft.get();

			finalIris = SegmentationWithOutputVideo(Temporary_elipsed(cv::Rect(	x1, y1, x2, y2)),
											(*imageSegmentationVideo)(cv::Rect(	x1, y1, x2, y2)));

		} else {

			finalIris = Segmentation(Temporary_elipsed(cv::Rect(x1, y1, x2, y2)));
		}

		cv::Mat blackAndWitheIris(cv::Size(eyeImage->size()), CV_8UC1);
		blackAndWitheIris.setTo(cv::Scalar(0,0,0));

		finalIris.copyTo(blackAndWitheIris(cv::Rect(x1, y1, x2, y2)));

		Histogram(blackAndWitheIris, *histogram_horizontal, *histogram_vertical, vector_horizontal, vector_vertical, histPositionSegmentedPixels);

		// Iris detected

		cv::Mat extraFeaturesImageSegmented(cv::Size(eyeImage->size()), CV_8UC1);

		cv::rectangle(extraFeaturesImageSegmented, cv::Point(0,0), cv::Point(eyeImage->size().width,eyeImage->size().height), cv::Scalar(GRAY_LEVEL, GRAY_LEVEL, GRAY_LEVEL), -1, 8);

		(*eyeGrey)(cvRect(x1, y1, x2, y2)).copyTo(extraFeaturesImageSegmented(cvRect(x1, y1, x2, y2)), finalIris);


		cv::Mat aux(eyeImage->size(), CV_8UC3);

		cv::rectangle(aux, cv::Point(0,0), cv::Point(eyeImage->size().width,eyeImage->size().height), cv::Scalar(0,255,0), -1, 8);

		aux(cv::Rect(x1, y1, x2, y2)).copyTo((*eyeImage)(cv::Rect(x1, y1, x2, y2)), finalIris);
	}


}

cv::Mat ExtractEyeFeaturesSegmentation::constructTemplateDisk(int sizeDisk) {

	CvPoint center;
	int radius = sizeDisk/2;
	center.x = floor(sizeDisk/2);
	center.y = floor(sizeDisk/2);

	CvScalar color;
	color = CV_RGB(0, 0, 0);


	cv::Mat irisTemplateDisk(cv::Size(sizeDisk, sizeDisk), CV_8UC1);

	irisTemplateDisk.setTo(cv::Scalar(255,255,255));

	cv::circle(irisTemplateDisk, center, radius, color, -1);

	return irisTemplateDisk;
}


// ME HE QUEDADO AQUI!!!

cv::Mat ExtractEyeFeaturesSegmentation::SegmentationWithOutputVideo(cv::Mat Temporary, cv::Mat image) {
	cv::Mat im_bw(Temporary.size(), CV_8UC1);
	
	// specify the range of colours that you want to include, you can play with the borders here
	cv::Scalar lowerb = cv::Scalar(0,255,0);
	cv::Scalar upperb = cv::Scalar(0,255,0); 

	cv::inRange(image, lowerb, upperb, im_bw); // if the frame has any orange pixel, this will be painted in the mask as white
	return im_bw;
}


cv::Mat ExtractEyeFeaturesSegmentation::Segmentation(cv::Mat Temporary) {

	cv::Mat im_bw(Temporary.size(), CV_8UC1);

	threshold(Temporary, im_bw, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	//threshold(Temporary, im_bw, CalculateOptimalThreshold(Temporary), 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	//threshold(Temporary, im_bw, CalculateOptimalThreshold(Temporary), 255, CV_THRESH_BINARY);

	bitwise_not(im_bw, im_bw);

	return im_bw;
}

int ExtractEyeFeaturesSegmentation::CalculateOptimalThreshold(cv::Mat Temporary) {

	std::vector<int> histograma_segmentacion(255, 0);

	int pos = -1;

	for (int y = 0; y < Temporary.size().height; y++) {
		for (int x = 0; x < Temporary.size().width; x++) {

			histograma_segmentacion.operator[](Temporary.data[Temporary.channels()*(Temporary.size().width*x + y) + 0])++;

			//histograma_segmentacion.operator[](cvGet2D(Temporary, y, x).val[0])++;

		}
	}

	int totalPixels = Temporary.size().height * Temporary.size().width;
	int optimalValue = Otsu(histograma_segmentacion, totalPixels);

	std::cout << "optimalValue: " << optimalValue << std::endl;

	return optimalValue;
}

int ExtractEyeFeaturesSegmentation::Otsu(std::vector<int> histogram, int total) {
    float sum = 0;
    for (int i = 1; i < 256; ++i)
        sum += i * histogram.operator[](i);
    float sumB = 0;
    int wB = 0;
    int wF = 0;
    double mB;
    double mF;
    float max = 0.0;
    double between = 0.0;
    int threshold1 = 0.0;
    int threshold2 = 0.0;
    for (int i = 0; i < 256; ++i) {
        wB += histogram.operator[](i);
        if (wB == 0)
            continue;
        wF = total - wB;
        if (wF == 0)
            break;
        sumB += i * histogram.operator[](i);
        mB = sumB / wB;
        mF = (sum - sumB) / wF;
        between = wB * wF * (mB - mF) * (mB - mF);
        if ( between >= max ) {
            threshold1 = i;
            if ( between > max ) {
                threshold2 = i;
            }
            max = between;            
        }
    }
    return ( threshold1 + threshold2 ) / 2.0;
}

cv::Mat ExtractEyeFeaturesSegmentation::CreateTemplateGausian2D(cv::Mat Gaussian2D) {

	CvPoint center;
	center.x = floor(Gaussian2D.size().width/2);
	center.y = floor(Gaussian2D.size().height/2);

	float tmp = 0;
	float sigma = 200;	// With this sigma, the furthest eye pixel (corners) have around 0.94 probability
	float max_prob = exp( - (0) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);

	for (int x = 0; x < Gaussian2D.size().width; x++) {
		for (int y = 0; y < Gaussian2D.size().height; y++) {
			float fx = abs(x - center.x);
			float fy = abs(y - center.y);

			tmp = exp( - (fx * fx + fy * fy) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);
			tmp = tmp / max_prob;	// Divide by max prob. so that values are in range [0, 1]

			Gaussian2D.data[Gaussian2D.channels()*(Gaussian2D.size().width*x + y) + 0] = tmp;

			//CvScalar tmp_scalar;
			//tmp_scalar.val[0] = tmp;

			//Gaussian2D.data[Gaussian2D.channels()*(Gaussian2D.cols*x + y) + 0] = tmp

			//cvSet2D(Gaussian2D, y, x, tmp_scalar);
			//cout << tmp << " ";
		}
		//cout << endl;
	}
	//cout << endl << endl;
	
	/*
    for(int row = 0; row < Gaussian2D->height; row++)
    {
        for(int col = 0; col < Gaussian2D->width; col++)
        {
            printf("%f, ", cvGet2D(Gaussian2D, row, col).val[0]);
        }
        printf("\n");
    }
    

	cin.get();
*/
	return Gaussian2D;
}

void ExtractEyeFeaturesSegmentation::Histogram(	cv::Mat blackAndWitheIris, 
														cv::Mat hist_horizontal, 
														cv::Mat hist_vertical, 
														std::vector<int> * blackAndWitheIris_summed_horizontal,
														std::vector<int> * blackAndWitheIris_summed_vertical,
														std::vector<std::vector<int> >* histPositionSegmentedPixels) {

	int width = blackAndWitheIris.step;
	//int width = blackAndWitheIris->widthStep;
	
	uchar* data = (uchar*) blackAndWitheIris.data;
	//uchar* data = (uchar*) blackAndWitheIris->imageData;

	blackAndWitheIris_summed_horizontal->assign(HORIZONTAL_BIN_SIZE, 0);
	blackAndWitheIris_summed_vertical->assign(VERTICAL_BIN_SIZE, 0);

	for (int i = 0; i < blackAndWitheIris.size().width; i++) {
	    for (int k = 0; k < blackAndWitheIris.size().height; k++) {
			if ((int) data[i + k*width] == 255) {	// PORQUE BLANCO?
				blackAndWitheIris_summed_horizontal->operator[](i)++;
				blackAndWitheIris_summed_vertical->operator[](k)++;
			}
	    }
	}

	cv::rectangle(hist_horizontal, cv::Point(0,0), cv::Point(hist_horizontal.size().width, hist_horizontal.size().height), cv::Scalar(0,0,0), -1, 8);

	for (int i = 0; i < blackAndWitheIris.size().width; i++) {
	    line(hist_horizontal,
	    	cvPoint(i, hist_horizontal.size().height),
		    cvPoint(i, hist_horizontal.size().height - blackAndWitheIris_summed_horizontal->operator[](i)),
		    CV_RGB(255,255,255));
	}

	cv::rectangle(hist_vertical, cv::Point(0,0), cv::Point(hist_vertical.size().width, hist_vertical.size().height), cv::Scalar(0,0,0), -1, 8);

	for (int i = 0; i < blackAndWitheIris.size().width; i++) {	// HE CAMBIADO width POR height
	    line(hist_vertical,
	    	cvPoint(i, hist_vertical.size().height),
		    cvPoint(i, hist_vertical.size().height - blackAndWitheIris_summed_vertical->operator[](i)),
		    CV_RGB(255,255,255));
	}

	SortHistogram(blackAndWitheIris_summed_horizontal, blackAndWitheIris_summed_vertical, histPositionSegmentedPixels);
}

void ExtractEyeFeaturesSegmentation::SortHistogram(	std::vector<int>* histHorizontalPixelsSegmented, 
													std::vector<int>* histVerticalPixelsSegmented, 
													std::vector<std::vector<int> >* histPositionSegmentedPixels) {

	int left_pos = 0, right_pos = 0, pos_mean = 0, acum = 0;
	bool bool_left_pos = false, bool_right_pos = false;

	std::cout << "RIGHT ACUM SORT" << std::endl;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if ((left_pos != histHorizontalPixelsSegmented->operator[](j)) && (bool_left_pos == false)) {
	        left_pos = j;
	        bool_left_pos = true;
	    }

	    if ((right_pos != histHorizontalPixelsSegmented->operator[](histHorizontalPixelsSegmented->size()-j)) && (bool_right_pos == false)) {
	        right_pos = histHorizontalPixelsSegmented->size()-j;
	        bool_right_pos = true;
	    }

	    acum += histHorizontalPixelsSegmented->operator[](j);
	}

	std::cout << "RIGHT MEAN SORT" << std::endl;

	acum = acum / 2;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if (acum > 0) {
	    	acum -= histHorizontalPixelsSegmented->operator[](j);
	    	if (acum <= 0) {
	    		pos_mean = j;
	    	}
	    }
	}

	std::cout << "RIGHT GAUSSIAN SORT" << std::endl;

	double sigma = 20;
	double mean = pos_mean;
	double diff = 0.0;

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {
		diff = mean-j;

		histHorizontalPixelsSegmented->operator[](j) = floor(histHorizontalPixelsSegmented->operator[](j) * exp( - diff*diff/ (2*sigma*sigma) ) );

	}




	bool_left_pos = false, left_pos = 0, bool_right_pos = false, right_pos = 0, pos_mean = 0, acum = 0;

	std::cout << "LEFT ACUM SORT" << std::endl;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if ((left_pos != histVerticalPixelsSegmented->operator[](j)) && (bool_left_pos == false)) {
	        left_pos = j;
	        bool_left_pos = true;
	    }

	    if ((right_pos != histVerticalPixelsSegmented->operator[](histVerticalPixelsSegmented->size()-j)) && (bool_right_pos == false)) {
	        right_pos = histVerticalPixelsSegmented->size()-j;
	        bool_right_pos = true;
	    }

	    acum += histVerticalPixelsSegmented->operator[](j);
	}

	std::cout << "LEFT MEAN SORT" << std::endl;

	acum = acum / 2;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if (acum > 0) {
	    	acum -= histVerticalPixelsSegmented->operator[](j);
	    	if (acum <= 0) {
	    		pos_mean = j;
	    	}
	    }
	}

	std::cout << "LEFT GAUSSIAN SORT" << std::endl;


	sigma = 20;
	mean = pos_mean;

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {
		diff = mean-j;

		histVerticalPixelsSegmented->operator[](j) = floor(histVerticalPixelsSegmented->operator[](j) * exp( - diff*diff/ (2*sigma*sigma) ) );
		
	}



	histPositionSegmentedPixels->clear();

	std::cout << "HIST" << std::endl;

	int max = -1;

	// Obtencion del maximo

	for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	    if (max < histHorizontalPixelsSegmented->operator[](j)) {
	        max = histHorizontalPixelsSegmented->operator[](j);
	    }
	}

	for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	    if (max < histVerticalPixelsSegmented->operator[](j)) {
	        max = histVerticalPixelsSegmented->operator[](j);
	    }
	}

	int aux_max = -1;

	// Colocacion de los elementos que contengan ese maximo en un vector

	std::vector<int> aux_histPosition;

	while (max > -1) {

	    for (int j = 0; j < histHorizontalPixelsSegmented->size(); j++) {

	        if (histHorizontalPixelsSegmented->operator[](j) == max) {

	            aux_histPosition.push_back(j);

	        } else if ((histHorizontalPixelsSegmented->operator[](j) < max) && 
	            (aux_max < histHorizontalPixelsSegmented->operator[](j))) {

	            aux_max = histHorizontalPixelsSegmented->operator[](j);
	        }
	    }

	    for (int j = 0; j < histVerticalPixelsSegmented->size(); j++) {

	        if (histVerticalPixelsSegmented->operator[](j) == max) {

	            aux_histPosition.push_back(j + histHorizontalPixelsSegmented->size());

	        } else if ((histVerticalPixelsSegmented->operator[](j) < max) && 
	            (aux_max < histVerticalPixelsSegmented->operator[](j))) {

	            aux_max = histVerticalPixelsSegmented->operator[](j);
	        }
	    }

	    histPositionSegmentedPixels->push_back(aux_histPosition);

		aux_histPosition.clear();

	    max = aux_max;
	    aux_max = -1;

	}

	std::cout << "histPositionSegmentedPixels: " << std::endl;

	for (int i = 0; i < histPositionSegmentedPixels->size(); i++) {
		for (int j = 0; j < histPositionSegmentedPixels->operator[](i).size(); j++) {
			std::cout << histPositionSegmentedPixels->operator[](i).operator[](j) << " ";
		}
		std::cout << std::endl;
	}


	std::cout << "Size of vector[1]: " << histPositionSegmentedPixels->size() << std::endl;

}

void ExtractEyeFeaturesSegmentation::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;
	
	cv::Mat image = Application::Components::videoInput->debugFrame;
	int eyeDX = eyeSize.width;
	int eyeDY = eyeSize.height;

	int baseX = 0;
	int baseY = 0;
	int stepX = 0;
	int stepY = 2*eyeDY;


	histogram_horizontal.copyTo(image(cv::Rect(baseX + stepX * 1, baseY + stepY * 2, eyeDX, eyeDY)));

	cv::Mat temp = histogram_vertical.t();
	temp.copyTo(image(cv::Rect(baseX + stepX * 1, baseY + stepY * 3, eyeDX, eyeDY)));

	histogram_horizontal_left.copyTo(image(cv::Rect(baseX + 100, baseY + stepY * 2, eyeDX, eyeDY)));

	temp = histogram_vertical_left.t();
	temp.copyTo(image(cv::Rect(baseX + 100, baseY + stepY * 3, eyeDX, eyeDY)));
	
}
