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
		boost::scoped_ptr<VideoInput> videoInput;
		boost::scoped_ptr<VideoWriter> video;
		
		MainGazeTracker *mainTracker;
		
		PointTracker *pointTracker;
		EyeExtractor *eyeExtractor;
		EyeExtractorSegmentationGroundTruth *eyeExtractorSegmentationGroundTruth;
		ExtractEyeFeaturesSegmentation *eyeSegmentation;
		EyeCenterDetector *eyeCenterDetector;
		GazeTracker *gazeTracker;
		GazeTrackerHistogramFeatures *gazeTrackerHistogramFeatures;
		HeadTracker *headTracker;
		HeadCompensation *headCompensation;
		Calibrator *calibrator;
		
		DebugWindow *debugWindow;
		TestWindow *testWindow;
		GoogleGlassWindow *googleGlassWindow;
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
			//stores.push_back(boost::shared_ptr<AbstractStore>(new WindowStore(WindowPointer::PointerSpec(20, 30, 0, 0, 1), WindowPointer::PointerSpec(20, 20, 0, 1, 1), WindowPointer::PointerSpec(30, 30, 1, 0, 1))));
		}

		return stores;
	}
}

