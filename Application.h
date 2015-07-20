#pragma once

#include <fstream>

#include "OutputMethods.h"
#include "Video.h"
#include "Calibrator.h"
#include "DebugWindow.h"
#include "HeadCompensation.h"
#include "PointTracker.h"
#include "EyeExtractor.h"
#include "EyeExtractorSegmentationGroundTruth.h"
#include "EyeCenterDetector.h"
#include "MainGazeTracker.h"
#include "TestWindow.h"
#include "GoogleGlassWindow.h"
#include "FrogGame.h"
#include "HistogramFeatureExtractor.h"
#include "GazeTrackerHistogramFeatures.h"
#include "AnchorPointSelector.h"
#include "PointTrackerWithTemplate.h"

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

		// The main class containing the processing loop
		extern MainGazeTracker *mainTracker;

		// Point selection component for initialization
		extern AnchorPointSelector *anchorPointSelector;

		// Preprocessing (before gaze estimation) components
		// that prepare the necessary data used in estimation
		extern PointTracker *pointTracker;
		extern PointTrackerWithTemplate *pointTrackerWithTemplate;
		extern HeadTracker *headTracker;
		extern HeadCompensation *headCompensation;
		extern EyeExtractor *eyeExtractor;
		extern EyeExtractorSegmentationGroundTruth *eyeExtractorSegmentationGroundTruth;
		extern EyeCenterDetector *eyeCenterDetector;
		extern HistogramFeatureExtractor *histFeatureExtractor;

		// Gaze tracker components
		extern GazeTracker *gazeTracker;
		extern GazeTrackerHistogramFeatures *gazeTrackerHistogramFeatures;

		// Other components mainly taking care of calibration/test flow and display
		extern Calibrator *calibrator;
		extern DebugWindow *debugWindow;
		extern TestWindow *testWindow;
		extern GoogleGlassWindow *googleGlassWindow;
		extern FrogGame *frogGame;
	}

	namespace Signals {
		extern int initiateCalibrationFrameNo;
		extern int initiateTestingFrameNo;
		extern int initiatePointSelectionFrameNo;
		extern int initiatePointClearingFrameNo;
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
