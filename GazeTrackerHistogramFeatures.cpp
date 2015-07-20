#include <fstream>

#include "GazeTrackerHistogramFeatures.h"
#include "HistogramFeatureExtractor.h"
#include "EyeExtractor.h"
#include "Point.h"
#include "mir.h"
#include "Application.h"

GazeTrackerHistogramFeatures::GazeTrackerHistogramFeatures()
{
	// Initialize calibration sample accumulation variables
	_currentTargetSamples.create(cv::Size(MAX_SAMPLES_PER_TARGET, FEATURE_DIM), CV_32FC1);
	_currentTargetSamplesLeft.create(cv::Size(MAX_SAMPLES_PER_TARGET, FEATURE_DIM), CV_32FC1);

	clearTargetSamples();

	// And the variables to hold the current sample
	_currentSample.create(cv::Size(1, FEATURE_DIM), CV_32FC1);
	_currentSampleLeft.create(cv::Size(1, FEATURE_DIM), CV_32FC1);

	Application::Data::gazePointHistFeaturesGP.x = 0;
	Application::Data::gazePointHistFeaturesGP.y = 0;
	Application::Data::gazePointHistFeaturesGPLeft.x = 0;
	Application::Data::gazePointHistFeaturesGPLeft.y = 0;
}

void GazeTrackerHistogramFeatures::process() {
	if (!Application::Components::pointTracker->isTrackingSuccessful()) {
		return;
	}

	// If recalibration is necessary (there is a new target), recalibrate the Gaussian Processes
	if(Application::Components::calibrator->needRecalibration) {
		addExemplar();
	}

	// Combine the current horizontal and vertical features to come up with the final features
	cv::Mat horizontalTransposed = Application::Components::histFeatureExtractor->horizontalFeatures.t();
	horizontalTransposed.copyTo(_currentSample(cv::Rect(0, 0, 1, HORIZONTAL_BIN_SIZE)));

	Application::Components::histFeatureExtractor->verticalFeatures.copyTo(_currentSample(cv::Rect(0, HORIZONTAL_BIN_SIZE, 1, VERTICAL_BIN_SIZE)));

	horizontalTransposed = Application::Components::histFeatureExtractor->horizontalFeaturesLeft.t();
	horizontalTransposed.copyTo(_currentSampleLeft(cv::Rect(0, 0, 1, HORIZONTAL_BIN_SIZE)));

	Application::Components::histFeatureExtractor->verticalFeaturesLeft.copyTo(_currentSampleLeft(cv::Rect(0, HORIZONTAL_BIN_SIZE, 1, VERTICAL_BIN_SIZE)));

	// If active and there is a usable frame
	if(Application::Components::calibrator->isActive()
		&& Application::Components::calibrator->getPointFrameNo() >= 11
		&& !Application::Components::eyeExtractor->isBlinking()) {
			// Copy the current histogram feature sample to the corresponding row in the accumulation matrices (currentTargetSamples)
			_currentSample.copyTo(_currentTargetSamples(cv::Rect(_currentTargetSampleCount, 0, 1, FEATURE_DIM)));
			_currentSampleLeft.copyTo(_currentTargetSamplesLeft(cv::Rect(_currentTargetSampleCount, 0, 1, FEATURE_DIM)));

			// Increment the sample counter
			_currentTargetSampleCount++;
	}

	// Update the left and right estimations
	updateEstimations();
}

void GazeTrackerHistogramFeatures::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;

	cv::Mat image = Application::Components::videoInput->debugFrame;

	// If not blinking, draw the estimations to debug window
	if (isActive() && !Application::Components::eyeExtractor->isBlinking()) {
		Point estimation(0, 0);

		cv::circle(image,
			Utils::mapFromMainScreenToDebugFrameCoordinates(cv::Point((Application::Data::gazePointHistFeaturesGP.x + Application::Data::gazePointHistFeaturesGPLeft.x) / 2, (Application::Data::gazePointHistFeaturesGP.y + Application::Data::gazePointHistFeaturesGPLeft.y) / 2)),
			8, cv::Scalar(255, 0, 0), -1, 8, 0);
	}
}

// Decides if the tracker is active
bool GazeTrackerHistogramFeatures::isActive() {
	return _histX.get() && _histY.get();
}

// Adds the exemplar for the current calibration target into the exemplars vectors
void GazeTrackerHistogramFeatures::addExemplar() {
	// Create the new sample that is added (create in heap)
	cv::Mat *exemplar 		= new cv::Mat(cv::Size(1, FEATURE_DIM), CV_32FC1);
	cv::Mat *exemplarLeft 	= new cv::Mat(cv::Size(1, FEATURE_DIM), CV_32FC1);

	// Calculate the average of collected samples for this target and store in exemplar
	cv::reduce(_currentTargetSamples(cv::Rect(0, 0, _currentTargetSampleCount, FEATURE_DIM)), 		*exemplar, 		1, CV_REDUCE_AVG, CV_32FC1);
	cv::reduce(_currentTargetSamplesLeft(cv::Rect(0, 0, _currentTargetSampleCount, FEATURE_DIM)), 	*exemplarLeft,	1, CV_REDUCE_AVG, CV_32FC1);

	// Add the exemplar to the targets vector
	_exemplars.push_back(*exemplar);
	_exemplarsLeft.push_back(*exemplarLeft);

	// Clear the used samples
	clearTargetSamples();

	trainGaussianProcesses();
}

// Uses the current sample to calculate the gaze estimation
void GazeTrackerHistogramFeatures::updateEstimations() {
	if (isActive()) {
		// Update estimations for right eye
		Application::Data::gazePointHistFeaturesGP.x = _histX->getmean(_currentSample);
		Application::Data::gazePointHistFeaturesGP.y = _histY->getmean(_currentSample);

		// Repeat for left eye
		Application::Data::gazePointHistFeaturesGPLeft.x = _histXLeft->getmean(_currentSampleLeft);
		Application::Data::gazePointHistFeaturesGPLeft.y = _histYLeft->getmean(_currentSampleLeft);

		Utils::boundToScreenArea(Application::Data::gazePointHistFeaturesGP);
		Utils::boundToScreenArea(Application::Data::gazePointHistFeaturesGPLeft);
	}
}

void GazeTrackerHistogramFeatures::trainGaussianProcesses() {
	std::vector<double> xLabels;
	std::vector<double> yLabels;

	// Prepare separate vector of targets for X and Y directions
	for (int i = 0; i < Application::Data::calibrationTargets.size(); i++) {
		xLabels.push_back(Application::Data::calibrationTargets[i].x);
		yLabels.push_back(Application::Data::calibrationTargets[i].y);
	}

	// Re-train the GP estimators with the exemplars for the calibration targets
	// and the corresponding X and Y labels
	_histX.reset(new HistProcess(_exemplars, xLabels, covarianceFunctionSE, 0.01));
	_histY.reset(new HistProcess(_exemplars, yLabels, covarianceFunctionSE, 0.01));

	_histXLeft.reset(new HistProcess(_exemplarsLeft, xLabels, covarianceFunctionSE, 0.01));
	_histYLeft.reset(new HistProcess(_exemplarsLeft, yLabels, covarianceFunctionSE, 0.01));
}

// Clears the matrix buffer where samples are added for each calibratin target
// Prepares the tracker for the next calibration target samples
void GazeTrackerHistogramFeatures::clearTargetSamples() {
	_currentTargetSamples.setTo(cv::Scalar(0,0,0));
	_currentTargetSamplesLeft.setTo(cv::Scalar(0,0,0));

	_currentTargetSampleCount = 0;
}

double GazeTrackerHistogramFeatures::covarianceFunctionSE(const cv::Mat &histogram1, const cv::Mat &histogram2) {
	const double sigma = 125.0; // 1.0;
    const double lscale = 250.0; // 500.00;

	// Calculate the squared L2 norm (sum of squared diff)
	double norm = cv::norm(histogram1, histogram2, cv::NORM_L2);
	norm = norm*norm;

	// Return the squared exponential kernel output
    return sigma*sigma*exp(-norm / (2*lscale*lscale) );
}

/*
// Covariance function for the GP gaze estimator for X axis
double GazeTrackerHistogramFeatures::covarianceFunctionOrderedX(cv::Mat histogram1, cv::Mat histogram2) {
	return covarianceFunctionBaseOrdered(histogram1, histogram2, 125.0, 250.0, 1, 1);
}

// Covariance function for the GP gaze estimator for Y axis
double GazeTrackerHistogramFeatures::covarianceFunctionOrderedY(cv::Mat histogram1, cv::Mat histogram2) {
	return covarianceFunctionBaseOrdered(histogram1, histogram2, 175.0, 1000.0, 1, 25);
}

// Base covariance function shared between X and Y axes
double GazeTrackerHistogramFeatures::covarianceFunctionBaseOrdered(cv::Mat histogram1, cv::Mat histogram2, double sigma, double lscale, int horizontalIncrement, int verticalIncrement) {


	// TODO ONUR Sort histograms according to the following, wherever necessary
	// SortHistogram(horizontalFeatures, verticalFeatures, histPositionSegmentedPixels);

	// Order the histograms and calculate the covariance between the ordered elements
	double norm = 0.0;

	int numberToLookFor;

	bool numberFound = false;

	int aux_hist1 = 0;
	int aux_hist2 = 0;

	double horizontal_hist1 = 0, vertical_hist1 = 0, horizontal_hist2 = 0, vertical_hist2 = 0;

	for (int i = 0; i < histogram1.size(); i++) {
		for (int j = 0; j < histogram1[i].size(); j++) {
			// Get the next number from the first histogram
			numberToLookFor = histogram1[i][j];

			// Search the number in the second histogram
			for (int e = 0; e < histogram2.size(); e++) {
				for (int r = 0; r < histogram2[e].size(); r++) {
					if (numberToLookFor == histogram2[e][r]) {
						// If number is found, add the
						if (histogram1[i][j] < 128) {
							norm += abs(horizontal_hist1 - horizontal_hist2);
						} else {
							norm += abs(vertical_hist1 - vertical_hist2);
						}

						numberFound = true;
						break;
					}
				} // End search in the second dimension

				// If the number is not found in this subvector (second dimension of histogram2)
				if (numberFound == false) {
					for (int r = 0; r < histogram2[e].size(); r++) {
						if (histogram2[e][r] < 128) {
							horizontal_hist2 += horizontalIncrement;
						} else {
							vertical_hist2 += verticalIncrement;
						}
					}
				} else {
					horizontal_hist2 = 0;
					vertical_hist2 = 0;
					aux_hist2 = 0;
					numberFound = false;
					break;
				}
			} // End search for the number
		} // End second dimension of histogram1

		for (int j = 0; j < histogram1[i].size(); j++) {
			if (histogram1[i][j] < 128) {
				horizontal_hist1 += horizontalIncrement;
			} else {
				vertical_hist1 += verticalIncrement;
			}
		}
	}

	norm = sigma*sigma*exp(-norm / (2*lscale*lscale) );

	return norm;
}
*/
