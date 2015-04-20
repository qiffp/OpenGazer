#include <fstream>

#include "GazeTracker.h"
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

/*
int Targets::getCurrentTarget(Point point) {
	std::vector<double> distances(targets.size());
	//debugTee(targets);
	transform(targets.begin(), targets.end(), distances.begin(), sigc::mem_fun(point, &Point::distance));
	//debugTee(distances);
	return min_element(distances.begin(), distances.end()) - distances.begin();
	//for (int i = 0; i < targets.size(); i++) {
	//	if (point.distance(targets[i]) < 30) {
	// 		return i;
	// 	}
	//}
	//return -1;
}

*/


GazeTracker::GazeTracker()
{
	Application::Data::gazePointGP.x = 0;
	Application::Data::gazePointGP.y = 0;
	Application::Data::gazePointGPLeft.x = 0;
	Application::Data::gazePointGPLeft.y = 0;
}

bool GazeTracker::isActive() {
	return _gaussianProcessX.get() && _gaussianProcessY.get();
}

void GazeTracker::clear() {
	// updateGaussianProcesses()
}

void GazeTracker::addExemplar() {
	// Add new sample to the GPs. Save the image samples (average eye images) in corresponding vectors
	_calibrationTargetImages.push_back(Application::Components::eyeExtractor->averageEye->getMean());
	_calibrationTargetImagesLeft.push_back(Application::Components::eyeExtractor->averageEyeLeft->getMean());
	
	updateGaussianProcesses();
}

void GazeTracker::removeCalibrationError(Point &estimate) {
	double x[1][2];
	double output[1];
	double sigma[1];
	int pointCount = Application::Data::calibrationTargets.size() + 4;

	if (_betaX == -1 && _gammaX == -1) {
		return;
	}

	x[0][0] = estimate.x;
	x[0][1] = estimate.y;

	//std::cout << "INSIDE CAL ERR REM. BETA = " << _betaX << ", " << _betaY << ", GAMMA IS " << _gammaX << ", " << _gammaY << std::endl;
	//for (int i = 0; i < pointCount; i++) {
	//	std::cout << _xv[i][0] << ", " << _xv[i][1] << std::endl;
	//}

	int N = pointCount;
	N = binomialInv(N, 2) - 1;

	//std::cout << "CALIB. ERROR REMOVAL. Target size: " << pointCount << ", " << N << std::endl;

	mirEvaluate(1, 2, 1, (double *)x, pointCount, (double *)_xv, _fvX, _sigv, 0, NULL, NULL, NULL, _betaX, _gammaX, N, 2, output, sigma);

	if (output[0] >= -100) {
		estimate.x = output[0];
	}

	mirEvaluate(1, 2, 1, (double *)x, pointCount, (double *)_xv, _fvY, _sigv, 0, NULL, NULL, NULL, _betaY, _gammaY, N, 2, output, sigma);

	if (output[0] >= -100) {
		estimate.y = output[0];
	}

	//std::cout << "Estimation corrected from: (" << x[0][0] << ", " << x[0][1] << ") to (" << estimate.x << ", " << estimate.y << ")" << std::endl;

	boundToScreenArea(estimate);

	//std::cout << "Estimation corrected from: (" << x[0][0] << ", " << x[0][1] << ") to (" << estimate.x << ", " << estimate.y << ")" << std::endl;
}

void GazeTracker::boundToScreenArea(Point &estimate) {
	cv::Rect *rect = Utils::getMainMonitorGeometry();

	// If x or y coordinates are outside screen boundaries, correct them
	if (estimate.x < rect->x) {
		estimate.x = rect->x;
	}

	if (estimate.y < rect->y) {
		estimate.y = rect->y;
	}

	if (estimate.x >= rect->x + rect->width) {
		estimate.x = rect->x + rect->width;
	}

	if (estimate.y >= rect->y + rect->height) {
		estimate.y = rect->y + rect->height;
	}
}

void GazeTracker::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;
	
	cv::Mat image = Application::Components::videoInput->debugFrame;

	// If not blinking, draw the estimations to debug window
	if (isActive() && !Application::Components::eyeExtractor->isBlinking()) {
		Point estimation(0, 0);
		
		//Utils::mapToVideoCoordinates(Application::Data::gazePointGP, Application::Components::videoInput->getResolution(), estimation);
		cv::circle(image, 
			Utils::mapFromMainScreenToDebugCoordinates(cv::Point(Application::Data::gazePointGP.x, Application::Data::gazePointGP.y)), 
			8, cv::Scalar(0, 255, 0), -1, 8, 0);

		//Utils::mapToVideoCoordinates(Application::Data::gazePointGPLeft, Application::Components::videoInput->getResolution(), estimation);
		cv::circle(image, 
			Utils::mapFromMainScreenToDebugCoordinates(cv::Point(Application::Data::gazePointGPLeft.x, Application::Data::gazePointGPLeft.y)), 
			8, cv::Scalar(255, 0, 0), -1, 8, 0);
	}
}

void GazeTracker::process() {
	if (!Application::Components::pointTracker->isTrackingSuccessful()) {
		return;
	}
	
	// If recalibration is necessary (there is a new target), recalibrate the Gaussian Processes
	if(Application::Components::calibrator->needRecalibration) {
		addExemplar();
	}
	
	if(Application::Components::calibrator->isActive()
		&& Application::Components::calibrator->getPointFrameNo() >= 11
		&& !Application::Components::eyeExtractor->isBlinking()) {
		
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
	}
	
	// Update the left and right estimations
	updateEstimations();
}

void GazeTracker::updateEstimations() {
	if (isActive()) {
		cv::Mat *image = Application::Components::eyeExtractor->eyeFloat.get();
		Application::Data::gazePointGP = Point(_gaussianProcessX->getmean(Utils::SharedImage(image, &ignore)), _gaussianProcessY->getmean(Utils::SharedImage(image, &ignore)));
		
		image = Application::Components::eyeExtractor->eyeFloatLeft.get();
		Application::Data::gazePointGPLeft = Point(_gaussianProcessXLeft->getmean(Utils::SharedImage(image, &ignore)), _gaussianProcessYLeft->getmean(Utils::SharedImage(image, &ignore)));

		// Bound estimations to screen area
		boundToScreenArea(Application::Data::gazePointGP);
		boundToScreenArea(Application::Data::gazePointGPLeft);
	}
}

Point GazeTracker::getTarget(int id) {
	return Application::Data::calibrationTargets[id];
}

int GazeTracker::getTargetId(Point point) {
	return point.closestPoint(Application::Data::calibrationTargets);
}

void GazeTracker::calculateTrainingErrors() {
	int j = 0;

	//std::cout << "Input count: " << _calibrationTargetPointsAllFrames.size();
	//std::cout << ", Target size: " << Application::Data::calibrationTargets.size() << std::endl;

	// For each calibration target, calculate the average estimation using calibration samples
	for (int i = 0; i < Application::Data::calibrationTargets.size(); i++) {
		double xTotal = 0;
		double yTotal = 0;
		double sampleCount = 0;
		
		while (j < _calibrationTargetPointsAllFrames.size() 
				&& Application::Data::calibrationTargets[i].x == _calibrationTargetPointsAllFrames[j].x
				&& Application::Data::calibrationTargets[i].y == _calibrationTargetPointsAllFrames[j].y) {
			double xEstimate = (_gaussianProcessX->getmean(_calibrationTargetImagesAllFrames[j]) + _gaussianProcessXLeft->getmean(_calibrationTargetImagesLeftAllFrames[j])) / 2;
			double yEstimate = (_gaussianProcessY->getmean(_calibrationTargetImagesAllFrames[j]) + _gaussianProcessYLeft->getmean(_calibrationTargetImagesLeftAllFrames[j])) / 2;

			//std::cout << "i, j = (" << i << ", " << j << "), est: " << xEstimate << "(" << _gaussianProcessX->getmean(SharedImage(allImages[j], &ignore)) << "," << _gaussianProcessXLeft->getmean(SharedImage(allImagesLeft[j], &ignore)) << ")" << ", " << yEstimate << "(" << _gaussianProcessY->getmean(SharedImage(allImages[j], &ignore)) << "," << _gaussianProcessYLeft->getmean(SharedImage(allImagesLeft[j], &ignore)) << ")" << std::endl;

			xTotal += xEstimate;
			yTotal += yEstimate;
			sampleCount++;
			j++;
		}

		xTotal /= sampleCount;
		yTotal /= sampleCount;

		Application::resultsOutputFile << "TARGET: (" << Application::Data::calibrationTargets[i].x << "\t, " << Application::Data::calibrationTargets[i].y << "\t),\tESTIMATE: (" << xTotal << "\t, " << yTotal << ")" << std::endl;
		//std::cout << "TARGET: (" << Application::Data::calibrationTargets[i].x << "\t, " << Application::Data::calibrationTargets[i].y << "\t),\tESTIMATE: (" << xTotal << "\t, " << yTotal << "),\tDIFF: (" << fabs(Application::Data::calibrationTargets[i].x- x_total) << "\t, " << fabs(Application::Data::calibrationTargets[i].y - y_total) << ")" << std::endl;

		// Calibration error removal
		_xv[i][0] = xTotal;		// Source
		_xv[i][1] = yTotal;

		// Targets
		_fvX[i] = Application::Data::calibrationTargets[i].x;
		_fvY[i] = Application::Data::calibrationTargets[i].y;
		_sigv[i] = 0;

		int targetId = getTargetId(Point(xTotal, yTotal));

		if (targetId != i) {
			std::cout << "Target id is not the expected one!! (Expected: "<< i << ", Current: " << targetId << ")" << std::endl;
		}
	}

	// Add the corners of the monitor as 4 extra data points. This helps the correction for points that are near the edge of monitor
	cv::Rect *rect = Utils::getMainMonitorGeometry();
	
	_xv[Application::Data::calibrationTargets.size()][0] = rect->x;
	_xv[Application::Data::calibrationTargets.size()][1] = rect->y;
	_fvX[Application::Data::calibrationTargets.size()] = rect->x-40;
	_fvY[Application::Data::calibrationTargets.size()] = rect->y-40;

	_xv[Application::Data::calibrationTargets.size()+1][0] = rect->x + rect->width;
	_xv[Application::Data::calibrationTargets.size()+1][1] = rect->y;
	_fvX[Application::Data::calibrationTargets.size()+1] = rect->x + rect->width + 40;
	_fvY[Application::Data::calibrationTargets.size()+1] = rect->y - 40;

	_xv[Application::Data::calibrationTargets.size()+2][0] = rect->x + rect->width;
	_xv[Application::Data::calibrationTargets.size()+2][1] = rect->y + rect->height;
	_fvX[Application::Data::calibrationTargets.size()+2] = rect->x + rect->width + 40;
	_fvY[Application::Data::calibrationTargets.size()+2] = rect->y + rect->height + 40;

	_xv[Application::Data::calibrationTargets.size()+3][0] = rect->x;
	_xv[Application::Data::calibrationTargets.size()+3][1] = rect->y + rect->height;
	_fvX[Application::Data::calibrationTargets.size()+3] = rect->x - 40;
	_fvY[Application::Data::calibrationTargets.size()+3] = rect->y + rect->height + 40;

	int pointCount = Application::Data::calibrationTargets.size() + 4;
	int N = pointCount;
	N = binomialInv(N, 2) - 1;

	// Find the best beta and gamma parameters for interpolation
	mirBetaGamma(1, 2, pointCount, (double *)_xv, _fvX, _sigv, 0, NULL, NULL, NULL, N, 2, 50.0, &_betaX, &_gammaX);
	mirBetaGamma(1, 2, pointCount, (double *)_xv, _fvY, _sigv, 0, NULL, NULL, NULL, N, 2, 50.0, &_betaY, &_gammaY);

	Application::resultsOutputFile << std::endl << std::endl;
	std::cout << std::endl << std::endl;

	Application::resultsOutputFile.flush();

	std::cout << "ERROR CALCULATION FINISHED. BETA = " << _betaX << ", " << _betaY << ", GAMMA IS " << _gammaX << ", " << _gammaY << std::endl;
	for (int i = 0; i < pointCount; i++) {
		std::cout << _xv[i][0] << ", " << _xv[i][1] << std::endl;
	}
}

void GazeTracker::printTrainingErrors() {
	//cv::Rect *rect = Utils::getMainMonitorGeometry();

	//std::cout << "PRINTING TRAINING ESTIMATIONS: " << std::endl;
	//for (int i = 0; i < 15; i++) {
	//	int imageIndex = 0;
	//	int j = 0;
	//	while (j < _calibrationTargetPointsAllFrames.size() && Application::Data::calibrationTargets[i].x == allOutputCoords[j][0] && Application::Data::calibrationTargets[i].y == allOutputCoords[j][1]) {
	//		std::cout << "X, Y: '" << _gaussianProcessX->getmean(SharedImage(allImages[j], &ignore)) << ", " << _gaussianProcessY->getmean(SharedImage(allImages[j], &ignore)) << "' and '" << _gaussianProcessXLeft->getmean(SharedImage(allImagesLeft[j], &ignore)) << ", " << _gaussianProcessYLeft->getmean(SharedImage(allImagesLeft[j], &ignore)) << "' " << std::endl;
	//		image_index++;
	//		j++;
	//	}
	//}
}

double GazeTracker::imageDistance(const cv::Mat *image1, const cv::Mat *image2) {
	double norm = cv::norm(*image1, *image2, CV_L2);
	return norm * norm;
}

double GazeTracker::covarianceFunction(Utils::SharedImage const &image1, Utils::SharedImage const &image2) {
	const double sigma = 1.0;
	const double lscale = 500.0;
	return sigma * sigma * exp(-imageDistance(image1.get(), image2.get()) / (2 * lscale * lscale));
}

void GazeTracker::updateGaussianProcesses() {
	std::vector<double> xLabels;
	std::vector<double> yLabels;

	// Prepare separate vector of targets for X and Y directions 
	for (int i = 0; i < Application::Data::calibrationTargets.size(); i++) {
		xLabels.push_back(Application::Data::calibrationTargets[i].x);
		yLabels.push_back(Application::Data::calibrationTargets[i].y);
	}
	
	_gaussianProcessX.reset(new ImProcess(_calibrationTargetImages, xLabels, covarianceFunction, 0.01));
	_gaussianProcessY.reset(new ImProcess(_calibrationTargetImages, yLabels, covarianceFunction, 0.01));
	
	_gaussianProcessXLeft.reset(new ImProcess(_calibrationTargetImagesLeft, xLabels, covarianceFunction, 0.01));
	_gaussianProcessYLeft.reset(new ImProcess(_calibrationTargetImagesLeft, yLabels, covarianceFunction, 0.01));
}

