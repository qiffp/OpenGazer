#pragma once

#include <boost/scoped_ptr.hpp>

#include "utils.h"
#include "GaussianProcess.cpp"

/*
struct TrackerOutput {
	Point gazePoint;
	Point gazePointLeft;

	// Neural network
	Point nnGazePoint;
	Point nnGazePointLeft;
	Point target;
	Point actualTarget;
	int targetId;
	int frameId;
	//bool outputError;

	TrackerOutput(Point gazePoint, Point target, int targetId);
	void setActualTarget(Point actual);
	//void setErrorOutput(bool show);
	void setFrameId(int id);
};
*/

class GazeTracker {
	typedef MeanAdjustedGaussianProcess<Utils::SharedImage> ImProcess;

public:
	GazeTracker();
	bool isActive();
	void clear();
	void addExemplar();

	// Calibration error removal
	void removeCalibrationError(Point &estimate);
	void boundToScreenArea(Point &estimate);

	void draw();
	void process();
	void updateEstimations();
	
	Point getTarget(int id);
	int getTargetId(Point point);
	void calculateTrainingErrors();
	void printTrainingErrors();

private:
	std::vector<Utils::SharedImage> _calibrationTargetImages;
	std::vector<Utils::SharedImage> _calibrationTargetImagesLeft;
	std::vector<Point> _calibrationTargetPoints;
	
	std::vector<Utils::SharedImage> _calibrationTargetImagesAllFrames;
	std::vector<Utils::SharedImage> _calibrationTargetImagesLeftAllFrames;
	std::vector<Point> _calibrationTargetPointsAllFrames;
	
	boost::scoped_ptr<ImProcess> _gaussianProcessX;
	boost::scoped_ptr<ImProcess> _gaussianProcessY;

	// ONUR DUPLICATED CODE FOR LEFT EYE
	boost::scoped_ptr<ImProcess> _gaussianProcessXLeft;
	boost::scoped_ptr<ImProcess> _gaussianProcessYLeft;

	// Calibration error removal
	double _betaX, _gammaX, _betaY, _gammaY, _sigv[100];	// Max 100 calibration points
	double _xv[100][2], _fvX[100], _fvY[100];

	static double imageDistance(const cv::Mat *image1, const cv::Mat *image2);
	static double covarianceFunction(const Utils::SharedImage &image1, const Utils::SharedImage &image2);

	void updateGaussianProcesses();
};
