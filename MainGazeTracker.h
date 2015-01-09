#pragma once

#include "TrackingSystem.h"
#include "Calibrator.h"
#include "OutputMethods.h"
#include "VideoInput.h"
#include "CommandLineArguments.h"

class MainGazeTracker {
public:
	boost::shared_ptr<TrackingSystem> trackingSystem;
	MovingTarget *target;
	FrameProcessing frameFunctions;
	boost::scoped_ptr<IplImage> canvas;
	IplImage *repositioningImage;
	WindowPointer *calibrationPointer;

	static MainGazeTracker &instance(int argc=0, char **argv=NULL);
	~MainGazeTracker();
	void process();
	void cleanUp();
	void startCalibration();
	void startTesting();
	void choosePoints();
	void clearPoints();
	void pauseOrRepositionHead();
	void extractFaceRegionRectangle(IplImage *frame, std::vector<Point> featurePoints);

private:
	boost::scoped_ptr<VideoInput> _videoInput;
	int _frameStoreLoad;
	std::vector<boost::shared_ptr<AbstractStore> > _stores;
	int _frameCount;
	std::string _directory;
	std::string _basePath;
	std::ofstream *_outputFile;
	IplImage *_conversionImage;
	IplImage *_overlayImage;
	std::vector<CvRect> _faces;
	Calibrator *_calibrator;
	int _headDistance;
	bool _videoOverlays;
	long _totalFrameCount;

	MainGazeTracker(int argc, char **argv);
};

