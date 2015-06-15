#include <fstream>

#include "MainGazeTracker.h"


#include "PointTracker.h"
#include "EyeExtractor.h"
#include "GazeTracker.h"
#include "GazeTrackerHistogramFeatures.h"
#include "HeadTracker.h"
#include "HeadCompensation.h"

#include "Application.h"
#include "utils.h"
#include "FaceDetector.h"
#include "Detection.h"
#include "DebugWindow.h"
#include "TestWindow.h"

#include "HiResTimer.h"

MainGazeTracker::MainGazeTracker(int argc, char **argv):
	_stores(Application::getStores()),
	_commandIndex(-1),
	_initiateCalibration(false),
	_initiateTesting(false),
	_initiatePointSelection(false),
	_initiatePointClearing(false)
{
	Application::Components::mainTracker = this;
	
	CommandLineArguments args(argc, argv);

	if (args.getOptionValue("help").length()) {
		std::cout << std::endl;
		std::cout << "CVC Machine Vision Group Eye-Tracker" << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "Usage:\teyetracker --subject=SUBJECT_NAME [--resolution=[480|720]] [--setup=SETUP_FOLDER_NAME] [--headdistance=DISTANCE] [--record=[0|1]] [--overlay=[0|1]] [--input=INPUT_FILE_PATH]" << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "OPTIONS:" << std::endl;
		std::cout << "\tsubject:\tSubject\'s name to be used in the output file name." << std::endl;
		std::cout << "\tresolution:\tResolution for the camera. 480 for 640x480 resolution, 720 for 1280x720." << std::endl;
		std::cout << "\tsetup:\t\tExperiment setup name and also the folder to read the test and calibration point locations." << std::endl;
		std::cout << "\theaddistance:\tSubject\'s head distance in cm to be included in the output file for automatic error calculation purposes." << std::endl;
		std::cout << "\trecord:\t\tWhether a video of the experiment should be recorded for offline processing purposes." << std::endl;
		std::cout << "\toverlay:\tWhether target point and estimation pointers are written as an overlay to the recorded video. Should not be used when offline processing is desired on the output video." << std::endl;
		std::cout << "\tinput:\t\tInput video path in case of offline processing." << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "SAMPLE USAGES:" << std::endl;
		std::cout << "\tBasic usage without video recording:" << std::endl;
		std::cout << "\t\t./eyetracker --subject=david --resolution=720 --setup=std --headdistance=80 --record=0" << std::endl;
		std::cout << std::endl;
		std::cout << "\tUsage during experiments to enable video recording for offline processing:" << std::endl;
		std::cout << "\t\t./eyetracker --subject=david --resolution=720 --setup=std --headdistance=80 --record=1" << std::endl;
		std::cout << std::endl;
		std::cout << "\tUsage during offline processing:" << std::endl;
		std::cout << "\t\t./eyetracker --subject=david --resolution=720 --setup=std --headdistance=80 --record=1 --overlay=1 --input=../outputs/david_std_720_1.avi" << std::endl;
		std::cout << std::endl;
		std::cout << "\tUsage during offline processing with lower resolution:" << std::endl;
		std::cout << "\t\t./eyetracker --subject=david --resolution=480 --setup=std --headdistance=80 --record=1 --overlay=1 --input=../outputs/david_std_720_1.avi" << std::endl;
		std::cout << std::endl << std::endl;
		exit(0);
	}

	if (args.getOptionValue("input").length()) {
		Application::Components::videoInput.reset(new VideoInput(args.getOptionValue("resolution"), args.getOptionValue("input"), true));

		// Read the commands (SELECT, CLEAR, CALIBRATE, TEST)
		std::string inputCommandFilename = args.getOptionValue("input");
		inputCommandFilename = inputCommandFilename.substr(0, inputCommandFilename.length()-4) + "_commands.txt";

		_commandInputFile.open(inputCommandFilename.c_str());

		for(;;) {
			long number;
			std::string name;
			_commandInputFile >> number >> name;

			if (_commandInputFile.rdstate()) {
				break; // break if any error
			}

			_commands.push_back(Command(number, name));
		}

		_commandIndex = 0;

		std::cout << _commands.size() << " commands read." << std::endl;
	} else {
		// --resolution parameter
		if (args.getOptionValue("resolution").length()) {
			Application::Components::videoInput.reset(new VideoInput(args.getOptionValue("resolution")));
		} else {
			Application::Components::videoInput.reset(new VideoInput("480"));
		}
	}

	std::string subject = args.getOptionValue("subject");
	std::string setup = args.getOptionValue("setup");

	if (subject.length() == 0) {
		subject = "default";
	}

	if (setup.length() == 0) {
		setup = "std";
	}

	if (args.getOptionValue("overlay") == "1") {
		Application::Settings::videoOverlays = true;
	}

	// --headdistance parameter
	if (args.getOptionValue("headdistance").length()) {
		_headDistance = atoi(args.getOptionValue("headdistance").c_str());
	} else {
		_headDistance = 70;
	}

	// --dwelltime parameter
	if (args.getOptionValue("dwelltime").length()) {
		Application::dwelltimeParameter = atoi(args.getOptionValue("dwelltime").c_str());
	} else {
		Application::dwelltimeParameter = 30;
	}

	// --testdwelltime parameter
	if (args.getOptionValue("testdwelltime").length()) {
		Application::testDwelltimeParameter = atoi(args.getOptionValue("testdwelltime").c_str());
	} else {
		Application::testDwelltimeParameter = 20;
	}

	// --sleep parameter
	if (args.getOptionValue("sleep").length()) {
		Application::sleepParameter = atoi(args.getOptionValue("sleep").c_str());
	 } else {
		Application::sleepParameter = 0;
	 }

	// --folder parameter
	std::string folderParameter = "outputs";

	if (args.getOptionValue("outputfolder").length()) {
		folderParameter = args.getOptionValue("outputfolder");
	}

	// --subject parameter
	_basePath = Utils::getUniqueFileName(folderParameter, subject + "_" + setup + "_" + args.getOptionValue("resolution"));

	// --record parameter
	if (args.getOptionValue("record") == "1") {
		Application::Components::video.reset(new VideoWriter(Application::Components::videoInput->size, _basePath.substr(0, _basePath.length() - 4) + ".avi"));
		Application::Settings::recording = true;
	}

	Application::resultsOutputFile.open((_basePath + "_").c_str());

	// First write the system time
	time_t currentTime = time(NULL);
	Application::resultsOutputFile << ctime(&currentTime) << std::endl;

	// Then write the setup parameters
	Application::resultsOutputFile << "--input=" << args.getOptionValue("input") << std::endl;
	Application::resultsOutputFile << "--record=" << args.getOptionValue("record") << std::endl;
	Application::resultsOutputFile << "--overlay=" << (Application::Settings::videoOverlays ? "true" : "false") << std::endl;
	Application::resultsOutputFile << "--headdistance=" << _headDistance << std::endl;
	Application::resultsOutputFile << "--resolution=" << args.getOptionValue("resolution") << std::endl;
	Application::resultsOutputFile << "--setup=" << setup << std::endl;
	Application::resultsOutputFile << "--subject=" << subject << std::endl << std::endl;

	// Finally the screen resolution
	cv::Rect *rect = Utils::getSecondaryMonitorGeometry();
	
	Application::resultsOutputFile << "Screen resolution: " << rect->width << " x " << rect->height << " (Position: "<< rect->x << ", "<< rect->y << ")" << std::endl << std::endl;
	Application::resultsOutputFile.flush();

	// If recording, create the file to write the commands for button events
	if (Application::Settings::recording) {
		std::string commandFileName = _basePath.substr(0, _basePath.length() - 4) + "_commands.txt";
		_commandOutputFile.open(commandFileName.c_str());
	}

	_directory = setup;

	// Create system components
	Application::Components::pointTracker = new PointTracker(Application::Components::videoInput->size);
	Application::Components::eyeExtractor = new EyeExtractor();
	Application::Components::eyeExtractorSegmentationGroundTruth = new EyeExtractorSegmentationGroundTruth();
	Application::Components::eyeSegmentation = new ExtractEyeFeaturesSegmentation();
	Application::Components::eyeCenterDetector = new EyeCenterDetector();
	Application::Components::gazeTracker = new GazeTracker();
	Application::Components::gazeTrackerHistogramFeatures = new GazeTrackerHistogramFeatures();
	Application::Components::headTracker = new HeadTracker();
	Application::Components::headCompensation = new HeadCompensation();
	Application::Components::calibrator = new Calibrator();
	Application::Components::testWindow = new TestWindow();
	std::cout << "Creating Glass window" << std::endl;
	Application::Components::googleGlassWindow = new GoogleGlassWindow();
	Application::Components::frogGame = new FrogGame();
	Application::Components::debugWindow = new DebugWindow();
	
	Application::Components::debugWindow->raise();

	// Load detector cascades
	Detection::loadCascades();
	
	// Create timer for initiating main loop
	connect(&_timer, SIGNAL (timeout()), this, SLOT (process()));
	_timer.start(1);
}

MainGazeTracker::~MainGazeTracker() {
	cleanUp();
}

void MainGazeTracker::process() {
	Application::Components::videoInput->updateFrame();

	const cv::Mat frame = Application::Components::videoInput->frame;

	// Wait a little so that the marker stays on the screen for a longer time
//	if ((Application::status == Application::STATUS_CALIBRATING || Application::status == Application::STATUS_TESTING) && !Application::Components::videoInput->captureFromVideo) {
//		usleep(Application::sleepParameter);
//	}

	if (Application::status != Application::STATUS_PAUSED) {	
		// Execute any pending signals (start calibration, etc.)
		simulateClicks();
		processActionSignals();
		
		// Process all components
		Application::Components::calibrator->process();
		Application::Components::testWindow->process();
		Application::Components::pointTracker->process();
		Application::Components::headTracker->process();
		Application::Components::eyeExtractor->process();
		//Application::Components::eyeExtractorSegmentationGroundTruth->process();
		Application::Components::eyeSegmentation->process();
		//Application::Components::eyeCenterDetector->process();
		Application::Components::gazeTracker->process();
		Application::Components::gazeTrackerHistogramFeatures->process();

		//Application::Components::googleGlassWindow->process();
		Application::Components::frogGame->process();


		// Draw components' debug information on debug image
		Application::Components::calibrator->draw();
		Application::Components::testWindow->draw();
		Application::Components::pointTracker->draw();
		Application::Components::headTracker->draw();
		Application::Components::eyeExtractor->draw();
		//Application::Components::eyeExtractorSegmentationGroundTruth->draw();
		Application::Components::eyeSegmentation->draw();
		//Application::Components::eyeCenterDetector->draw();
		Application::Components::gazeTracker->draw();
		Application::Components::gazeTrackerHistogramFeatures->draw();

		//Application::Components::googleGlassWindow->draw();
		Application::Components::frogGame->draw();

		
		// Display debug image in the window
		Application::Components::debugWindow->display();
	}

	if ( (Application::Components::gazeTracker != NULL) && (Application::Components::gazeTracker->isActive()) ||
		(Application::Components::gazeTrackerHistogramFeatures != NULL) && (Application::Components::gazeTrackerHistogramFeatures->isActive()) ) {
		// Write the output to all the channels
		xForEach(iter, _stores) {
			(*iter)->store();
		}

		// Write the same info to the output text file
		if (Application::resultsOutputFile != NULL) {
			if (Application::status == Application::STATUS_TESTING) {
				if (!Application::Components::eyeExtractor->isBlinking()) {
					Application::resultsOutputFile << Application::Components::testWindow->getPointFrameNo() + 1 << "\t"
						<< Application::Components::testWindow->getActivePoint().x << "\t" << Application::Components::testWindow->getActivePoint().y << "\t"
						//<< Application::Data::gazePointGP.x 	<< "\t" << Application::Data::gazePointGP.y 	<< "\t"
						//<< Application::Data::gazePointGPLeft.x << "\t" << Application::Data::gazePointGPLeft.y
						<< Application::Data::gazePointHistFeaturesGP.x 	<< "\t" << Application::Data::gazePointHistFeaturesGP.y 	<< "\t"
						<< Application::Data::gazePointHistFeaturesGPLeft.x << "\t" << Application::Data::gazePointHistFeaturesGPLeft.y
						<< std::endl;
				} else {
					Application::resultsOutputFile << Application::Components::testWindow->getPointFrameNo() + 1 << "\t"
						<< Application::Components::testWindow->getActivePoint().x << "\t" << Application::Components::testWindow->getActivePoint().y << "\t"
						<< 0 << "\t" << 0 << "\t"
						<< 0 << "\t" << 0
						<< std::endl;
				}
			}

			Application::resultsOutputFile.flush();
		}
	}

	// If video output is requested
	if (Application::Settings::recording) {
		if (Application::Settings::videoOverlays) {
			Application::Components::video->write(Application::Components::videoInput->debugFrame);
		}
		else {
			Application::Components::video->write(Application::Components::videoInput->frame);	
		}
	}
}

void MainGazeTracker::simulateClicks() {
	if (_commands.size() > 0) {
		while(_commandIndex >= 0 && _commandIndex <= (_commands.size() - 1) && _commands[_commandIndex].frameNumber == Application::Components::videoInput->frameCount) {
			std::cout << "Command: " << _commands[_commandIndex].commandName << std::endl;
			if(strcmp(_commands[_commandIndex].commandName.c_str(), "SELECT") == 0) {
				std::cout << "Choosing points automatically" << std::endl;
				choosePoints();
			}
			else if(strcmp(_commands[_commandIndex].commandName.c_str(), "CLEAR") == 0) {
				std::cout << "Clearing points automatically" << std::endl;
				clearPoints();
			}
			else if(strcmp(_commands[_commandIndex].commandName.c_str(), "CALIBRATE") == 0) {
				std::cout << "Calibrating automatically" << std::endl;
				startCalibration();
			}
			else if(strcmp(_commands[_commandIndex].commandName.c_str(), "TEST") == 0) {
				std::cout << "Testing automatically" << std::endl;
				startTesting();
			}

			_commandIndex++;
		}

		//if (_commandIndex == _commands.size() && (Application::status == Application::STATUS_IDLE || Application::status == Application::STATUS_CALIBRATED)) {
		//	throw Utils::QuitNow();
		//}
	}
}

// Process the signals for initiating certain tracker processes (calibration, testing, etc.)
void MainGazeTracker::processActionSignals() {
	// If should start calibration
	if(_initiateCalibration) {
		std::cout << "Choosing calibration" << std::endl;
		
		// Write action to commands file
		if (Application::Settings::recording) {
			_commandOutputFile << Application::Components::videoInput->frameCount << " CALIBRATE" << std::endl;
		}
		
		// Set application state
		Application::status = Application::STATUS_CALIBRATING;
		
		// Read calibration target list and start the calibration
		std::ifstream calfile((_directory + "/calpoints.txt").c_str());
		std::vector<Point> points = Utils::readAndScalePoints(calfile);

		Application::Components::calibrator->start(points);
		Application::Components::eyeExtractor->start();
		
		// Clear signal
		_initiateCalibration = false;
	}
	
	// If shoud start testing
	if(_initiateTesting) {
		std::cout << "Starting testing" << std::endl;
		
		// Write action to commands file
		if (Application::Settings::recording) {
			_commandOutputFile << Application::Components::videoInput->frameCount << " TEST" << std::endl;
		}

		// Set application state
		Application::status = Application::STATUS_TESTING;
		
		// Read test target list and start testing
		std::ifstream targetFile((_directory + "/testpoints.txt").c_str());
		std::vector<Point> points = Utils::readAndScalePoints(targetFile);

		Application::Components::testWindow->start(points);
		
		Application::resultsOutputFile << "TESTING" << std::endl << std::endl;
		
		// Clear signal
		_initiateTesting = false;
	}
	
	// If should choose points in next frame
	if(_initiatePointSelection) {
		std::cout << "Choosing points" << std::endl;
		
		// Write action to commands file
		if (Application::Settings::recording) {
			_commandOutputFile << Application::Components::videoInput->frameCount << " SELECT" << std::endl;
		}
		
		// Choose points using the detector
		Detection::choosePoints();
		
		_initiatePointSelection = false;
	}
	
	// If should clear points in next frame
	if(_initiatePointClearing) {
		std::cout << "Clearing points" << std::endl;
		// Write action to commands file
		if (Application::Settings::recording) {
			_commandOutputFile << Application::Components::videoInput->frameCount << " CLEAR" << std::endl;
		}

		Application::Components::pointTracker->clearTrackers();

		_initiatePointClearing = false;
	}
}

void MainGazeTracker::cleanUp() {
	Application::resultsOutputFile.close();
	rename((_basePath + "_").c_str(), _basePath.c_str());
}

void MainGazeTracker::startCalibration() {
	_initiateCalibration = true;
}

void MainGazeTracker::startTesting() {
	_initiateTesting = true;
}

void MainGazeTracker::choosePoints() {
	_initiatePointSelection = true;
}

void MainGazeTracker::clearPoints() {
	_initiatePointClearing = true;
}



