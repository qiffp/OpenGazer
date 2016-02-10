#pragma once

#include <boost/scoped_ptr.hpp>

class BlinkDetector {
 public:
	BlinkDetector();
	void update(const cv::Mat &image);
	bool isBlinking;

private:
	boost::scoped_ptr<cv::Mat> _averageEye;
	//LambdaAccumulator _accumulator;
	//LinearStateSystem _states;
	bool _initialized;

	//static std::vector<StateNode> constructStates();
};
