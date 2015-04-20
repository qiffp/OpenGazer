#pragma once

#include "BlinkDetector.h"
#include "FeatureDetector.h"

class EyeExtractor {
public:
	static const int eyeDX;
	static const int eyeDY;
	static const cv::Size eyeSize;
	
	boost::scoped_ptr<FeatureDetector> averageEye;
	boost::scoped_ptr<FeatureDetector> averageEyeLeft;

	boost::scoped_ptr<cv::Mat> eyeGrey, eyeFloat, eyeImage;
	boost::scoped_ptr<cv::Mat> eyeGreyLeft, eyeFloatLeft, eyeImageLeft;
	
	EyeExtractor();
	~EyeExtractor();
	void process();
	bool isBlinking();
	bool hasValidSample();
	void draw();

	void start();
	void pointStart();
	void pointEnd();
	void abortCalibration();
	void calibrationEnded();
	
private:
	BlinkDetector _blinkDetector;
	BlinkDetector _blinkDetectorLeft;
	bool _isBlinking;

	void extractEye(const cv::Mat originalImage);
	void extractEyeLeft(const cv::Mat originalImage);
};

