#pragma once

#include <opencv/highgui.h>

class VideoInput {
public:
	IplImage *frame;
	CvSize resolution;

	VideoInput();
	VideoInput(std::string res);
	~VideoInput();
	void updateFrame();

private:
	CvCapture *_capture;
	long _lastFrameTime;
	int _frameCount;
};

