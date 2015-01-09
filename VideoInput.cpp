#include <sys/time.h>

#include "VideoInput.h"

VideoInput::VideoInput():
	_capture(cvCaptureFromCAM(0)),
	_frameCount(0)
{
	timeval time;
	gettimeofday(&time, NULL);
	_lastFrameTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);

	frame = cvQueryFrame(_capture);
	resolution = cvSize(frame->width, frame->height);
	cvFlip(frame, frame, 1);
}

VideoInput::VideoInput(std::string res):
	_capture(cvCaptureFromCAM(0)),
	_frameCount(0)
{
	if (res == "720") {
		cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH, 1280);
		cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT, 720);
	} else if (res == "1080") {
		cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH, 1920);
		cvSetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT, 1080);
	}

	timeval time;
	gettimeofday(&time, NULL);
	_lastFrameTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);

	frame = cvQueryFrame(_capture);
	resolution = cvSize(frame->width, frame->height);
	cvFlip(frame, frame, 1);
}


VideoInput::~VideoInput() {
	cvReleaseCapture(&_capture);
}

void VideoInput::updateFrame() {
	static double videoResolution = cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT);
	static double trackerResolution = frame->height;

	_frameCount++;
	if (_frameCount != 1) {
		frame = cvQueryFrame(_capture);
	}

	cvFlip(frame, frame, 1);
}

