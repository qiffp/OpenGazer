#include "Application.h"

namespace Application {
	trackerStatus status = STATUS_IDLE;
	bool isTrackerCalibrated = false;
	int dwelltimeParameter = 20;
	int testDwelltimeParameter = 20;
	int sleepParameter = 0;
	std::ofstream resultsOutputFile;

	namespace Settings {
		bool videoOverlays = false;
		bool recording = false;
	}

	namespace Components {
		// Video input and output
		boost::scoped_ptr<VideoInput> videoInput;
		boost::scoped_ptr<VideoWriter> video;

		// The main class containing the processing loop
		MainGazeTracker *mainTracker;

		// Point selection component for initialization
		AnchorPointSelector *anchorPointSelector;

		// Preprocessing (before gaze estimation) components
		// that prepare the necessary data used in estimation
		PointTracker *pointTracker;
		PointTrackerWithTemplate *pointTrackerWithTemplate;
		HeadTracker *headTracker;
		HeadCompensation *headCompensation;
		EyeExtractor *eyeExtractor;
		EyeExtractorSegmentationGroundTruth *eyeExtractorSegmentationGroundTruth;
		EyeCenterDetector *eyeCenterDetector;
		HistogramFeatureExtractor *histFeatureExtractor;

		// Gaze tracker components
		GazeTracker *gazeTracker;
		GazeTrackerHistogramFeatures *gazeTrackerHistogramFeatures;

		// Other components mainly taking care of calibration/test flow and display
		Calibrator *calibrator;
		DebugWindow *debugWindow;
		TestWindow *testWindow;
		GoogleGlassWindow *googleGlassWindow;
		FrogGame *frogGame;
	}

	namespace Signals {
		int initiateCalibrationFrameNo = -1;
		int initiateTestingFrameNo = -1;
		int initiatePointSelectionFrameNo = -1;
		int initiatePointClearingFrameNo = -1;
	}

	namespace Data {
		std::vector<Point> calibrationTargets;

		Point gazePointGP;
		Point gazePointGPLeft;

		// Outputs for Histogram Features Gaussian Process estimator
		Point gazePointHistFeaturesGP;
		Point gazePointHistFeaturesGPLeft;

		Point gazePointNN;
		Point gazePointNNLeft;


	}

	std::vector<boost::shared_ptr<AbstractStore> > getStores() {
		static std::vector<boost::shared_ptr<AbstractStore> > stores;

		if (stores.size() < 1) {
			stores.push_back(boost::shared_ptr<AbstractStore>(new SocketStore()));
			//stores.push_back(boost::shared_ptr<AbstractStore>(new StreamStore(std::cout)));
		}

		return stores;
	}
}
