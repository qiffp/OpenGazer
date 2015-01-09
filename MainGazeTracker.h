#pragma once

#include "TrackingSystem.h"
#include "Calibrator.h"
#include "OutputMethods.h"
#include "Video.h"
#include "Command.h"

class MainGazeTracker {
public:
	bool isCalibrationOutputWritten;
	boost::shared_ptr<TrackingSystem> trackingSystem;
	MovingTarget *target;
	FrameProcessing frameFunctions;
	boost::scoped_ptr<IplImage> canvas;
	boost::scoped_ptr<VideoInput> videoInput;
	IplImage *repositioningImage;
	WindowPointer *calibrationPointer;

	static MainGazeTracker &instance(int argc=0, char **argv=NULL);
	~MainGazeTracker();
	void process();
	void simulateClicks();
	void cleanUp();
	void addTracker(Point point);
	void addExemplar(Point exemplar);
	void startCalibration();
	void startTesting();
	void choosePoints();
	void clearPoints();
	void pauseOrRepositionHead();
	void extractFaceRegionRectangle(IplImage *frame, std::vector<Point> featurePoints);

private:
	boost::scoped_ptr<VideoWriter> _video;
	int _frameStoreLoad;
	std::vector<boost::shared_ptr<AbstractStore> > _stores;
	int _frameCount;
	bool _autoReload;
	std::string _directory;
	std::string _basePath;
	std::ofstream *_outputFile;
	std::ofstream *_commandOutputFile;
	std::ifstream *_commandInputFile;
	IplImage *_conversionImage;
	IplImage *_overlayImage;
	std::vector<CvRect> _faces;
	Calibrator *_calibrator;
	int _headDistance;
	bool _videoOverlays;
	long _totalFrameCount;
	bool _recording;
	std::vector<Command> _commands;
	int _commandIndex;

	MainGazeTracker(int argc, char **argv);
};

