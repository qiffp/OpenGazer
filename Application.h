#pragma once

#include <fstream>

#include "OutputMethods.h"
#include "Video.h"
#include "Calibrator.h"
#include "DebugWindow.h"
#include "HeadCompensation.h"
#include "GazeTrackerHistogramFeatures.h"
#include "PointTracker.h"
#include "EyeExtractor.h"
#include "EyeExtractorSegmentationGroundTruth.h"
#include "ExtractEyeFeaturesSegmentation.h"
#include "EyeCenterDetector.h"
#include "MainGazeTracker.h"
#include "TestWindow.h"
#include "GoogleGlassWindow.h"
#include "FrogGame.h"
#include "HistogramFeatureExtractor.h"

namespace Application {
	// Tracker status
	enum trackerStatus {
		STATUS_IDLE			= 1,	// App is not doing anything
		STATUS_CALIBRATED	= 2,	// App is not doing anything, but is calibrated
		STATUS_CALIBRATING	= 11,	// App is calibrating
		STATUS_TESTING		= 12,	// App is testing
		STATUS_PAUSED		= 13	// App is paused
	};

	extern trackerStatus status;
	extern bool isTrackerCalibrated;
	extern int dwelltimeParameter;
	extern int testDwelltimeParameter;
	extern int sleepParameter;
	extern std::ofstream resultsOutputFile;
	
	
	namespace Settings {
		extern bool videoOverlays;
		extern bool recording;
	}

	namespace Components {
		extern boost::scoped_ptr<VideoInput> videoInput;
		extern boost::scoped_ptr<VideoWriter> video;
		
		extern MainGazeTracker *mainTracker;
		
		extern PointTracker *pointTracker;
		extern EyeExtractor *eyeExtractor;
		extern EyeExtractorSegmentationGroundTruth *eyeExtractorSegmentationGroundTruth;
		extern ExtractEyeFeaturesSegmentation *eyeSegmentation;
		extern EyeCenterDetector *eyeCenterDetector;
		extern GazeTracker *gazeTracker;
		extern GazeTrackerHistogramFeatures *gazeTrackerHistogramFeatures;
		extern HeadTracker *headTracker;
		extern HeadCompensation *headCompensation;
		extern Calibrator *calibrator;
		
		extern DebugWindow *debugWindow;
		extern TestWindow *testWindow;
		extern GoogleGlassWindow *googleGlassWindow;
		extern FrogGame *frogGame;
		
		extern HistogramFeatureExtractor *histFeatureExtractor;
	}

	namespace Data {
		extern std::vector<Point> calibrationTargets;
		
		// Outputs for Gaussian Process estimator
		extern Point gazePointGP;
		extern Point gazePointGPLeft;

		// Outputs for Histogram Features Gaussian Process estimator
		extern Point gazePointHistFeaturesGP;
		extern Point gazePointHistFeaturesGPLeft;

		// Outputs for Neural Network estimator
		extern Point gazePointNN;
		extern Point gazePointNNLeft;


	}

	std::vector<boost::shared_ptr<AbstractStore> > getStores();
}

