#include <fstream>

#include "GazeTrackerHistogramFeatures.h"
#include "EyeExtractor.h"
#include "Point.h"
#include "mir.h"
#include "Application.h"


template <class T, class S> std::vector<S> getSubVector(std::vector<T> const &input, S T::*ptr) {
	std::vector<S> output(input.size());

	for (int i = 0; i < input.size(); i++) {
		output[i] = input[i].*ptr;
	}

	return output;
}

static void ignore(const cv::Mat *) {
}

GazeTrackerHistogramFeatures::GazeTrackerHistogramFeatures()
{
	Application::Data::gazePointHistFeaturesGP.x = 0;
	Application::Data::gazePointHistFeaturesGP.y = 0;
	Application::Data::gazePointHistFeaturesGPLeft.x = 0;
	Application::Data::gazePointHistFeaturesGPLeft.y = 0;
}

bool GazeTrackerHistogramFeatures::isActive() {
	return _histX.get() && _histY.get();
}

void GazeTrackerHistogramFeatures::addExemplar() {
	
	//ARCADI addExemplar
	
	int sizeVectorOfVectors = vectorOfVectors_horizontal.size();

    for (int i = sizeVectorOfVectors-1; i > Application::Components::calibrator->getPointNumber(); i--) {

        int sizeVector = vectorOfVectors_horizontal.operator[](i).size();

        for (int j = 0; j < sizeVector; j++) {

            vectorOfVectors_horizontal.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) += vectorOfVectors_horizontal.operator[](i).operator[](j);
            vectorOfVectors_horizontal_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) += vectorOfVectors_horizontal_left.operator[](i).operator[](j);
            
        }

        sizeVector = vectorOfVectors_vertical.operator[](i).size();

        for (int j = 0; j < sizeVector; j++) {

            vectorOfVectors_vertical.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) += vectorOfVectors_vertical.operator[](i).operator[](j);
            vectorOfVectors_vertical_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) += vectorOfVectors_vertical_left.operator[](i).operator[](j);
            
        }

        vectorOfVectors_horizontal.pop_back();
        vectorOfVectors_vertical.pop_back();
        vectorOfVectors_horizontal_left.pop_back();
        vectorOfVectors_vertical_left.pop_back();

    }
    std::cout << "Calibrator process() for 2" << std::endl;

    for (int j = 0; j < vectorOfVectors_horizontal.operator[](Application::Components::calibrator->getPointNumber()).size(); j++) {

        vectorOfVectors_horizontal.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) = floor(vectorOfVectors_horizontal.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) / (sizeVectorOfVectors - Application::Components::calibrator->getPointNumber()));
        vectorOfVectors_horizontal_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) = floor(vectorOfVectors_horizontal_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) / (sizeVectorOfVectors - Application::Components::calibrator->getPointNumber()));
    }

    std::cout << "Calibrator process() for 3" << std::endl;
    for (int j = 0; j < vectorOfVectors_vertical.operator[](Application::Components::calibrator->getPointNumber()).size(); j++) {

        vectorOfVectors_vertical.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) = floor(vectorOfVectors_vertical.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) / (sizeVectorOfVectors - Application::Components::calibrator->getPointNumber()));
        vectorOfVectors_vertical_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) = floor(vectorOfVectors_vertical_left.operator[](Application::Components::calibrator->getPointNumber()).operator[](j) / (sizeVectorOfVectors - Application::Components::calibrator->getPointNumber()));
    
    }

    std::cout << "Calibrator process() resto" << std::endl;
    std::vector<std::vector<int> > AUX_POSITION_VECTOR;

    std::cout << "Calibrator process() resto 2" << std::endl;
    
	Application::Components::eyeSegmentation->SortHistogram(&(vectorOfVectors_horizontal.operator[](Application::Components::calibrator->getPointNumber())), &(vectorOfVectors_vertical.operator[](Application::Components::calibrator->getPointNumber())), &AUX_POSITION_VECTOR);

    std::cout << "Calibrator process() resto 3" << std::endl;
    histPositionSegmentedPixels.push_back(AUX_POSITION_VECTOR);

    std::cout << "Calibrator process() resto 4" << std::endl;
    std::vector<std::vector<int> > AUX_POSITION_VECTOR_LEFT;

    std::cout << "Calibrator process() resto 5" << std::endl;
	Application::Components::eyeSegmentation->SortHistogram(&(vectorOfVectors_horizontal_left.operator[](Application::Components::calibrator->getPointNumber())), &(vectorOfVectors_vertical_left.operator[](Application::Components::calibrator->getPointNumber())), &AUX_POSITION_VECTOR_LEFT);

    std::cout << "Calibrator process() resto 6" << std::endl;
    histPositionSegmentedPixels_left.push_back(AUX_POSITION_VECTOR_LEFT);
	
	//ARCADI addExemplar


/*
	// Add new sample to the GPs. Save the image samples (average eye images) in corresponding vectors
	_calibrationTargetHistogramFeatures.push_back(Application::Components::eyeExtractor->averageEye->getMean());
	_calibrationTargetHistogramFeaturesLeft.push_back(Application::Components::eyeExtractor->averageEyeLeft->getMean());
*/
	updateGaussianProcesses();
}

void GazeTrackerHistogramFeatures::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;
	
	cv::Mat image = Application::Components::videoInput->debugFrame;

	// If not blinking, draw the estimations to debug window
	if (isActive() && !Application::Components::eyeExtractor->isBlinking()) {
		Point estimation(0, 0);
		
		/*
		//Utils::mapToVideoCoordinates(Application::Data::gazePointGP, Application::Components::videoInput->getResolution(), estimation);
		cv::circle(image, 
			Utils::mapFromMainScreenToDebugFrameCoordinates(cv::Point(Application::Data::gazePointHistFeaturesGP.x, Application::Data::gazePointHistFeaturesGP.y)), 
			8, cv::Scalar(0, 255, 0), -1, 8, 0);

		//Utils::mapToVideoCoordinates(Application::Data::gazePointGPLeft, Application::Components::videoInput->getResolution(), estimation);
		cv::circle(image, 
			Utils::mapFromMainScreenToDebugFrameCoordinates(cv::Point(Application::Data::gazePointHistFeaturesGPLeft.x, Application::Data::gazePointHistFeaturesGPLeft.y)), 
			8, cv::Scalar(255, 0, 0), -1, 8, 0);

		*/

		// MIXED ESTIMATION

		//Utils::mapToVideoCoordinates(Application::Data::gazePointGP, Application::Components::videoInput->getResolution(), estimation);
		cv::circle(image, 
			Utils::mapFromMainScreenToDebugFrameCoordinates(cv::Point((Application::Data::gazePointHistFeaturesGP.x + Application::Data::gazePointHistFeaturesGPLeft.x) / 2, (Application::Data::gazePointHistFeaturesGP.y + Application::Data::gazePointHistFeaturesGPLeft.y) / 2)), 
			8, cv::Scalar(255, 0, 0), -1, 8, 0);

	}
}

void GazeTrackerHistogramFeatures::process() {

	if (!Application::Components::pointTracker->isTrackingSuccessful()) {
		return;
	}
	

	// Segmentacion, histogram extraction
	// If recalibration is necessary (there is a new target), recalibrate the Gaussian Processes
	if(Application::Components::calibrator->needRecalibration) {
		std::cout << "HIST FEATURES NEED RECALIB!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
		addExemplar();
	}

	if(Application::Components::calibrator->isActive()
		&& Application::Components::calibrator->getPointFrameNo() >= 11
		&& !Application::Components::eyeExtractor->isBlinking()) {

		// ARCADI PROCESS
        vectorOfVectors_horizontal.push_back(Application::Components::eyeSegmentation->vector_horizontal);
        vectorOfVectors_vertical.push_back(Application::Components::eyeSegmentation->vector_vertical);
        vectorOfVectors_horizontal_left.push_back(Application::Components::eyeSegmentation->vector_horizontal_left);
        vectorOfVectors_vertical_left.push_back(Application::Components::eyeSegmentation->vector_vertical_left);
        // ARCADI PROCESS

/*
		// Add current sample (not the average, but sample from each usable frame) to the vector
		cv::Mat *temp = new cv::Mat(EyeExtractor::eyeSize, CV_32FC1);
		Application::Components::eyeExtractor->eyeFloat->copyTo(*temp);

		Utils::SharedImage temp2(new cv::Mat(temp->size(), temp->type()), Utils::releaseImage);
		_calibrationTargetImagesAllFrames.push_back(temp2);
		
		// Repeat for left eye
		temp = new cv::Mat(EyeExtractor::eyeSize, CV_32FC1);
		Application::Components::eyeExtractor->eyeFloatLeft->copyTo(*temp);

		Utils::SharedImage temp3(new cv::Mat(temp->size(), temp->type()), Utils::releaseImage);
		_calibrationTargetImagesLeftAllFrames.push_back(temp3);
		
		_calibrationTargetPointsAllFrames.push_back(Application::Components::calibrator->getActivePoint());

*/
	}
	
	// Update the left and right estimations
	updateEstimations();
}

void GazeTrackerHistogramFeatures::updateEstimations() {
	std::cout << "Inside update estimations" << std::endl;
	if (isActive()) {
		std::cout << "Active" << std::endl;

		vector_h_v_combined = Application::Components::eyeSegmentation->histPositionSegmentedPixels;

		//ARCADI updateEstimations
		Application::Data::gazePointHistFeaturesGP = Point(_histX->getmean(vector_h_v_combined), 
								_histY->getmean(vector_h_v_combined));


		/* FALTA POR MIGRAR ESTA PARTE DEL CODIGO!!

		output.targetid = getTargetId(output.gazepoint);
		output.target = getTarget(output.targetid);
		
		*/


		vector_h_v_combined = Application::Components::eyeSegmentation->histPositionSegmentedPixels_left;

		//ARCADI updateEstimations left eye
		Application::Data::gazePointHistFeaturesGPLeft = Point(_histXLeft->getmean(vector_h_v_combined), 
				 				_histYLeft->getmean(vector_h_v_combined));
	std::cout << "Updated estimations" << std::endl;
		//ARCADI updateEstimations

		Utils::boundToScreenArea(Application::Data::gazePointHistFeaturesGP);
		Utils::boundToScreenArea(Application::Data::gazePointHistFeaturesGPLeft);

	std::cout << "Bound to scr coords" << std::endl;
/*
		cv::Mat *image = Application::Components::eyeExtractor->eyeFloat.get();
		Application::Data::gazePointGP = Point(_gaussianProcessX->getmean(Utils::SharedImage(image, &ignore)), _gaussianProcessY->getmean(Utils::SharedImage(image, &ignore)));
		
		image = Application::Components::eyeExtractor->eyeFloatLeft.get();
		Application::Data::gazePointGPLeft = Point(_gaussianProcessXLeft->getmean(Utils::SharedImage(image, &ignore)), _gaussianProcessYLeft->getmean(Utils::SharedImage(image, &ignore)));

		// Bound estimations to screen area
		Utils::boundToScreenArea(Application::Data::gazePointGP);
		Utils::boundToScreenArea(Application::Data::gazePointGPLeft);
*/
	}
	else {
	std::cout << "Not active" << std::endl;
	}
}

Point GazeTrackerHistogramFeatures::getTarget(int id) {
	return Application::Data::calibrationTargets[id];
}

int GazeTrackerHistogramFeatures::getTargetId(Point point) {
	return point.closestPoint(Application::Data::calibrationTargets);
}

double GazeTrackerHistogramFeatures::histDistancePosition_x(std::vector<std::vector<int> > histogram1, std::vector<std::vector<int> > histogram2) {

	std::cout << "histogram1: " << std::endl;

	for (int i = 0; i < histogram1.size(); i++) {
		for (int j = 0; j < histogram1.operator[](i).size(); j++) {
			std::cout << histogram1.operator[](i).operator[](j) << " ";
		}
		std::cout << std::endl;
	}

	std::cout << "histogram2: " << std::endl;

	for (int i = 0; i < histogram2.size(); i++) {
		for (int j = 0; j < histogram2.operator[](i).size(); j++) {
			std::cout << histogram2.operator[](i).operator[](j) << " ";
		}
		std::cout << std::endl;
	}

	//std::cin.get();



    const double sigma = 125.0;
    const double lscale = 250.0;

	double norm = 0.0;

	int NumberToLookFor;
	
	bool trobat = false;

	int aux_hist1 = 0;
	int aux_hist2 = 0;

	double horizontal_hist1 = 0, vertical_hist1 = 0, horizontal_hist2 = 0, vertical_hist2 = 0;

    for (int i = 0; i < histogram1.size(); i++) {
    	for (int j = 0; j < histogram1.operator[](i).size(); j++) {

    		NumberToLookFor = histogram1.operator[](i).operator[](j);

    		for (int e = 0; e < histogram2.size(); e++) {

    			for (int r = 0; r < histogram2.operator[](e).size(); r++) {

    				if (NumberToLookFor == histogram2.operator[](e).operator[](r)) {
    					if (histogram1.operator[](i).operator[](j) < 128) {
    						norm += abs(horizontal_hist1 - horizontal_hist2);
    					} else {
							norm += abs(vertical_hist1 - vertical_hist2);
    					}
    					trobat = true;
    					break;
    				}
    			}

    			if (trobat == false) {
    				for (int r = 0; r < histogram2.operator[](e).size(); r++) {
    					if (histogram2.operator[](e).operator[](r) < 128) {
							horizontal_hist2 +=1;
						} else {
							vertical_hist2 += 1;
						}
					}
    			} else {
    				horizontal_hist2 = 0;
    				vertical_hist2 = 0;
    				aux_hist2 = 0;
    				trobat = false;
    				break;
    			}
    		}
    	}
    	
		for (int j = 0; j < histogram1.operator[](i).size(); j++) {
			if (histogram1.operator[](i).operator[](j) < 128) {
				horizontal_hist1 += 1;
			} else {
				vertical_hist1 += 1;
			}
		}
    }

    std::cout << "norm: " << norm << std::endl;

    norm = sigma*sigma*exp(-norm / (2*lscale*lscale) );

    return norm;
}

double GazeTrackerHistogramFeatures::histDistancePosition_y(std::vector<std::vector<int> > histogram1, std::vector<std::vector<int> > histogram2) {
 
    const double sigma =175.0;
    const double lscale = 1000.0;

	double norm = 0.0;

	int NumberToLookFor;
	
	bool trobat = false;

	int aux_hist1 = 0;
	int aux_hist2 = 0;

	double horizontal_hist1 = 0, vertical_hist1 = 0, horizontal_hist2 = 0, vertical_hist2 = 0;

    for (int i = 0; i < histogram1.size(); i++) {
    	for (int j = 0; j < histogram1.operator[](i).size(); j++) {

    		NumberToLookFor = histogram1.operator[](i).operator[](j);

    		for (int e = 0; e < histogram2.size(); e++) {

    			for (int r = 0; r < histogram2.operator[](e).size(); r++) {

    				if (NumberToLookFor == histogram2.operator[](e).operator[](r)) {
    					if (histogram1.operator[](i).operator[](j) < 128) {
    						norm += abs(horizontal_hist1 - horizontal_hist2);
    					} else {
							norm += abs(vertical_hist1 - vertical_hist2);
    					}
    					trobat = true;
    					break;
    				}
    			}

    			if (trobat == false) {
    				for (int r = 0; r < histogram2.operator[](e).size(); r++) {
    					if (histogram2.operator[](e).operator[](r) < 128) {
							horizontal_hist2 += 1;
						} else {
							vertical_hist2 += 25;
						}
					}
    			} else {
    				horizontal_hist2 = 0;
    				vertical_hist2 = 0;
    				aux_hist2 = 0;
    				trobat = false;
    				break;
    			}
    		}
    	}
    	
		for (int j = 0; j < histogram1.operator[](i).size(); j++) {
			if (histogram1.operator[](i).operator[](j) < 128) {
				horizontal_hist1 += 1;
			} else {
				vertical_hist1 += 25;
			}
		}
    }

    std::cout << "norm: " << norm << std::endl;

    norm = sigma*sigma*exp(-norm / (2*lscale*lscale) );

    return norm;
}

double GazeTrackerHistogramFeatures::covariancefunction_hist_position_x(std::vector<std::vector<int> > const& histogram1, std::vector<std::vector<int> > const& histogram2)
{
    return histDistancePosition_x(histogram1, histogram2);
}

double GazeTrackerHistogramFeatures::covariancefunction_hist_position_y(std::vector<std::vector<int> > const& histogram1, std::vector<std::vector<int> > const& histogram2)
{
    return histDistancePosition_y(histogram1, histogram2);
}

void GazeTrackerHistogramFeatures::updateGaussianProcesses() {
	std::vector<double> xLabels;
	std::vector<double> yLabels;

	// Prepare separate vector of targets for X and Y directions 
	for (int i = 0; i < Application::Data::calibrationTargets.size(); i++) {
		xLabels.push_back(Application::Data::calibrationTargets[i].x);
		yLabels.push_back(Application::Data::calibrationTargets[i].y);
	}
	
	_histX.reset(new HistProcess(histPositionSegmentedPixels, xLabels, covariancefunction_hist_position_x, 0.01));
	_histY.reset(new HistProcess(histPositionSegmentedPixels, yLabels, covariancefunction_hist_position_y, 0.01));
	
	_histXLeft.reset(new HistProcess(histPositionSegmentedPixels_left, xLabels, covariancefunction_hist_position_x, 0.01));
	_histYLeft.reset(new HistProcess(histPositionSegmentedPixels_left, yLabels, covariancefunction_hist_position_y, 0.01));
}

