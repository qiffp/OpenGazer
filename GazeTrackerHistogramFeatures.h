#pragma once

#include <boost/scoped_ptr.hpp>
#include "utils.h"
#include "GaussianProcess.cpp"

#define FEATURE_DIM 192
#define MAX_SAMPLES_PER_TARGET 30

class GazeTrackerHistogramFeatures {
	typedef MeanAdjustedGaussianProcess<cv::Mat> HistProcess;

public:
	GazeTrackerHistogramFeatures();

	bool isActive();
	void addExemplar();

	// Calibration error removal
	void removeCalibrationError(Point &estimate);

	void draw();
	void process();
	void updateEstimations();

private:
	cv::Mat _currentTargetSamples, _currentTargetSamplesLeft;
	cv::Mat _currentSample, _currentSampleLeft;
	std::vector<cv::Mat> _exemplars, _exemplarsLeft;
	int _currentTargetSampleCount;

	// Gaussian Process estimators for right and left eye and both directions (X, Y)
	boost::scoped_ptr<HistProcess> _histX, _histY, _histXLeft, _histYLeft;

	//std::vector<Point> _calibrationTargetPointsAllFrames;	// ???

	static double covarianceFunctionSE(const cv::Mat &histogram1, const cv::Mat &histogram2);

	/*
	static double covarianceFunctionOrderedX(cv::Mat histogram1, cv::Mat histogram2);
	static double covarianceFunctionOrderedY(cv::Mat histogram1, cv::Mat histogram2);
	static double covarianceFunctionBaseOrdered(cv::Mat histogram1, cv::Mat histogram2, double sigma, double lscale, int horizontalIncrement, int verticalIncrement);
	*/

	void trainGaussianProcesses();
	void clearTargetSamples();
};
