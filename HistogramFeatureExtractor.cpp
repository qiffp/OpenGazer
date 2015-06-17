#include "HistogramFeatureExtractor.h"
#include "EyeExtractor.h"
#include "Application.h"
#include <opencv/highgui.h>
#include <math.h>
#include <fstream>
#define HORIZONTAL_BIN_SIZE 128
#define VERTICAL_BIN_SIZE 64

#define GRAY_LEVEL 127

static std::vector<int> vector_static_horizontal(HORIZONTAL_BIN_SIZE, 0);
static std::vector<int> vector_static_vertical(VERTICAL_BIN_SIZE, 0);

HistogramFeatureExtractor::HistogramFeatureExtractor()
{
	elipse_gray = cv::imread("./elipse.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	
	sizeImageDisk = 40;
	for (int j=0; j<VECTOR_SIZE; j++){
		constructTemplateDisk(j, sizeImageDisk+j*2);
		Gaussian2D[j].create(cv::Size(EyeExtractor::eyeSize.width - irisTemplateDisk[j].size().width + 1, EyeExtractor::eyeSize.height - irisTemplateDisk[j].size().height + 1), CV_32FC1);
		
		CreateTemplateGausian2D(j);
		Matches[j].create(Gaussian2D[j].size(), CV_32FC1);
	}
	
	this->histogram_horizontal.create(cv::Size(EyeExtractor::eyeSize), CV_8UC3);
	this->histogram_vertical.create(cv::Size(EyeExtractor::eyeSize.height, EyeExtractor::eyeSize.width), CV_8UC3);
	this->histogram_horizontal_left.create(cv::Size(EyeExtractor::eyeSize), CV_8UC3);
	this->histogram_vertical_left.create(cv::Size(EyeExtractor::eyeSize.height, EyeExtractor::eyeSize.width), CV_8UC3);
	
	this->wholeSegmentation.create(cv::Size(EyeExtractor::eyeSize), CV_8UC1);
	this->wholeSegmentation_left.create(cv::Size(EyeExtractor::eyeSize), CV_8UC1);
}

HistogramFeatureExtractor::~HistogramFeatureExtractor() {
}

void HistogramFeatureExtractor::process() {

	if (Application::Components::pointTracker->isTrackingSuccessful()) {

    	processToExtractFeatures();
    }
}

void HistogramFeatureExtractor::processToExtractFeatures(){
	for(int side = 0; side < 2; side++) {
		cv::Mat *eyeGrey 	= (side == 0) ? Application::Components::eyeExtractor->eyeGrey.get() : Application::Components::eyeExtractor->eyeGreyLeft.get();
		cv::Mat *eyeImage 	= (side == 0) ? Application::Components::eyeExtractor->eyeImage.get() : Application::Components::eyeExtractor->eyeImageLeft.get();
		std::vector<int> *vector_horizontal 	= (side == 0) ? &this->vector_horizontal 	: &this->vector_horizontal_left;
		std::vector<int> *vector_vertical 		= (side == 0) ? &this->vector_vertical 		: &this->vector_vertical_left;
		cv::Mat *histogram_horizontal 			= (side == 0) ? &this->histogram_horizontal : &this->histogram_horizontal_left;
		cv::Mat *histogram_vertical 			= (side == 0) ? &this->histogram_vertical 	: &this->histogram_vertical_left;
		cv::Mat *wholeSegmentation				= (side == 0) ? &this->wholeSegmentation	: &this->wholeSegmentation_left;
		cv::Mat Temporary_elipsed(eyeGrey->size(), CV_8UC1);
		
		Temporary_elipsed.setTo(cv::Scalar(100,100,100));
		eyeGrey->copyTo(Temporary_elipsed, elipse_gray);
		
		int j;
		
		int comparison_method = CV_TM_CCOEFF_NORMED; //CV_TM_CCOEFF_NORMED;

		for (j=0; j<VECTOR_SIZE; j++){
			matchTemplate(Temporary_elipsed, irisTemplateDisk[j], Matches[j], comparison_method);

			Matches[j] = Matches[j].mul(Gaussian2D[j]);

			minMaxLoc(Matches[j], &MinVal[j], &MaxVal[j], &MinLoc[j], &MaxLoc[j]);
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
		
		cv::Rect roi = cv::Rect(MaxLoc[i].x, MaxLoc[i].y, irisTemplateDisk[i].size().width, irisTemplateDisk[i].size().height);
		Segmentation(Temporary_elipsed, *wholeSegmentation);
		
		cv::Mat blackAndWitheIris(eyeImage->size(),CV_8UC1);
		
		blackAndWitheIris.setTo(cv::Scalar(0));
		
		(*wholeSegmentation)(roi).copyTo(blackAndWitheIris(roi));
		
		Histogram(blackAndWitheIris, *histogram_horizontal, *histogram_vertical, vector_horizontal, vector_vertical);
		
		// Iris detected
		/*cv::Mat extraFeaturesImageSegmented(cv::Size(eyeImage->size()), CV_8UC1);
		extraFeaturesImageSegmented.setTo(cv::Scalar(GRAY_LEVEL,GRAY_LEVEL,GRAY_LEVEL));
		
		(*eyeGrey)(roi).copyTo(extraFeaturesImageSegmented(roi), finalIris);*/
	
		cv::Mat aux(eyeImage->size(), CV_8UC3);
		aux.setTo(cv::Scalar(0,255,0));		// Fill with green
	
		aux(roi).copyTo((*eyeImage)(roi), blackAndWitheIris(roi));
	}

}

void HistogramFeatureExtractor::constructTemplateDisk(int index, int sizeDisk) {
	irisTemplateDisk[index].create(cv::Size(sizeDisk, sizeDisk), CV_8UC1);
	
	cv::Point center;
	int radius = sizeDisk/2;
	center.x = floor(radius);
	center.y = floor(radius);

	irisTemplateDisk[index].setTo(cv::Scalar(255,255,255));

	cv::circle(irisTemplateDisk[index], center, radius, cv::Scalar(0, 0, 0), -1);
}


void HistogramFeatureExtractor::Segmentation(cv::Mat Temporary, cv::Mat result) {

	threshold(Temporary, result, 80, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	bitwise_not(result, result);
}

void HistogramFeatureExtractor::CreateTemplateGausian2D(int index) {
	cv::Point center;
	center.x = floor(Gaussian2D[index].size().width/2);
	center.y = floor(Gaussian2D[index].size().height/2);

	float tmp = 0;
	float sigma = 200;	// With this sigma, the furthest eye pixel (corners) have around 0.94 probability
	float max_prob = exp( - (0) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);

	
	for (int x = 0; x < Gaussian2D[index].size().width; x++) {
		for (int y = 0; y < Gaussian2D[index].size().height; y++) {
			float fx = abs(x - center.x);
			float fy = abs(y - center.y);

			tmp = exp( - (fx * fx + fy * fy) / (2.0 * sigma * sigma)) / (2.0 * M_PI * sigma * sigma);
			tmp = tmp / max_prob;	// Divide by max prob. so that values are in range [0, 1]

			Gaussian2D[index].at<float>(cv::Point(x,y)) = tmp;
		}
	}
}

void HistogramFeatureExtractor::Histogram(	cv::Mat blackAndWitheIris, 
														cv::Mat hist_horizontal, 
														cv::Mat hist_vertical, 
														std::vector<int> * blackAndWitheIris_summed_horizontal,
														std::vector<int> * blackAndWitheIris_summed_vertical) {

	int width = blackAndWitheIris.step;
	uchar* data = (uchar*) blackAndWitheIris.data;
	
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

	hist_horizontal.setTo(cv::Scalar(0,0,0));

	for (int i = 0; i < blackAndWitheIris.size().width; i++) {
	    line(hist_horizontal,
	    	cv::Point(i, hist_horizontal.size().height),
		    cv::Point(i, hist_horizontal.size().height - blackAndWitheIris_summed_horizontal->operator[](i)),
		    cv::Scalar(255,255,255));
	}
	
	hist_vertical.setTo(cv::Scalar(0,0,0));

	for (int i = 0; i < blackAndWitheIris.size().width; i++) {	// HE CAMBIADO width POR height
	    line(hist_vertical,
	    	cv::Point(i, hist_vertical.size().height),
		    cv::Point(i, hist_vertical.size().height - blackAndWitheIris_summed_vertical->operator[](i)),
		    cv::Scalar(255,255,255));
	}
}


void HistogramFeatureExtractor::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;
	
	cv::Mat image = Application::Components::videoInput->debugFrame;
	int eyeDX = EyeExtractor::eyeSize.width;
	int eyeDY = EyeExtractor::eyeSize.height;

	int baseX = 0;
	int baseY = 0;
	int stepX = 0;
	int stepY = eyeDY+10;


	histogram_horizontal.copyTo(image(cv::Rect(baseX, baseY + stepY * 2, eyeDX, eyeDY)));

	cv::Mat temp = histogram_vertical.t();
	temp.copyTo(image(cv::Rect(baseX + stepX * 1, baseY + stepY * 3, eyeDX, eyeDY)));

	histogram_horizontal_left.copyTo(image(cv::Rect(baseX + 140, baseY + stepY * 2, eyeDX, eyeDY)));

	temp = histogram_vertical_left.t();
	temp.copyTo(image(cv::Rect(baseX + 140, baseY + stepY * 3, eyeDX, eyeDY)));

	// Draw the segmentations too
	Application::Components::eyeExtractor->eyeImage->copyTo(image(cv::Rect(baseX, baseY + stepY, eyeDX, eyeDY)));
	Application::Components::eyeExtractor->eyeImageLeft->copyTo(image(cv::Rect(baseX + 140, baseY + stepY, eyeDX, eyeDY)));
}




