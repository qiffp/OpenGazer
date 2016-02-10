#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>

#include "BlinkDetector.h"
#include "EyeExtractor.h"

BlinkDetector::BlinkDetector():
	_averageEye(new cv::Mat(EyeExtractor::eyeSize, CV_32FC1)),
	//_accumulator(0.1, 1000.0),
	//_states(constructStates()),
	_initialized(false),
    isBlinking(false)
{
}

void BlinkDetector::update(const cv::Mat &eyeFloat) {
	if (!_initialized) {
		eyeFloat.copyTo(*_averageEye.get());
		_initialized = true;
	}

	double distance = cv::norm(eyeFloat, *_averageEye, CV_L2);
    double norm = cv::norm(*_averageEye, CV_L2);
    
    // Normalize distance by the norm of the previous eye image
    distance = distance / norm;
    
    isBlinking = (distance > 0.18);
    
    //std::cout << "Distance is " << distance << std::endl;
    
    eyeFloat.copyTo(*_averageEye.get());
	//cv::accumulateWeighted(*eyeFloat, *_averageEye, 0.05); //TODO Maybe use running average of eye image
}
