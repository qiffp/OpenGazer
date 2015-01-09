#include <fstream>

#include "MainGazeTracker.h"
#include "Application.h"
#include "utils.h"
#include "FaceDetector.h"
#include "Detection.h"

namespace {
	float calculateDistance(CvPoint2D32f pt1, CvPoint2D32f pt2 ) {
		float dx = pt2.x - pt1.x;
		float dy = pt2.y - pt1.y;

		return cvSqrt((float)(dx * dx + dy * dy));
	}

	std::vector<Point> scaleByScreen(const std::vector<Point> &points) {
		int numMonitors = Gdk::Screen::get_default()->get_n_monitors();
		Gdk::Rectangle rect;
		Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

		if (numMonitors == 1) {
			return Calibrator::scaled(points, screen->get_width(), screen->get_height());
		} else {
			screen->get_monitor_geometry(numMonitors - 1, rect);
			return Calibrator::scaled(points, rect.get_width(), rect.get_height());
		}
	}

	void checkRectSize(IplImage *image, CvRect *rect) {
		if (rect->x < 0) {
			rect->x = 0;
		}

		if (rect->y < 0) {
			rect->y = 0;
		}

		if (rect->x + rect->width >= image->width) {
			rect->width = image->width - rect->x - 1;
		}

		if (rect->y + rect->height >= image->height) {
			rect->height = image->height - rect->y - 1;
		}
	}
}

MainGazeTracker &MainGazeTracker::instance(int argc, char **argv) {
	static MainGazeTracker *instance = NULL;

	if (argc && argv) {
		instance = new MainGazeTracker(argc, argv);
	} else {
		if (instance == NULL) {
			std::cout << "No existing MainGazeTracker instance - need to supply arguments to create one" << std::endl;
			exit(1);
		}
	}

	return *instance;
}

MainGazeTracker::MainGazeTracker(int argc, char **argv):
	_frameStoreLoad(-1),
	_stores(Application::getStores()),
	_totalFrameCount(0)
{
	CommandLineArguments args(argc, argv);

	if (args.getOptionValue("help").length()) {
		std::cout << std::endl;
		std::cout << "CVC Machine Vision Group Eye-Tracker" << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "Usage:\t./opengazer [--subject=SUBJECT_NAME] [--resolution=[480|720|1080]] [--setup=SETUP_FOLDER_NAME] [--headdistance=DISTANCE]" << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "OPTIONS:" << std::endl;
		std::cout << "\tsubject:\tSubject\'s name to be used in the output file name. (default: 'default')" << std::endl;
		std::cout << "\tresolution:\tResolution for the camera. 480 for 640x480 resolution, 720 for 1280x720, 1080 for 1920x1080. (default: 480)" << std::endl;
		std::cout << "\tsetup:\t\tExperiment setup name and also the folder to read the test and calibration point locations. (default: 'std')" << std::endl;
		std::cout << "\theaddistance:\tSubject\'s head distance in cm to be included in the output file for automatic error calculation purposes. (default: 70)" << std::endl;
		std::cout << std::endl << std::endl;
		std::cout << "SAMPLE USAGE:" << std::endl;
		std::cout << "\t./opengazer --subject=david --resolution=720 --setup=std --headdistance=80" << std::endl;
		std::cout << std::endl << std::endl;
		exit(0);
	}

	if (args.getOptionValue("resolution").length()) {
		_videoInput.reset(new VideoInput(args.getOptionValue("resolution")));
	} else {
		_videoInput.reset(new VideoInput());
	}

	_conversionImage = cvCreateImage(_videoInput->resolution, 8, 3);

	std::string subject = args.getOptionValue("subject");
	subject = subject.length() > 0 ? subject : "default";

	std::string setup = args.getOptionValue("setup");
	setup = setup.length() > 0 ? setup : "std";

	std::string outputFolder = args.getOptionValue("outputfolder");
	outputFolder = outputFolder.length() > 0 ? outputFolder : "outputs";

	// --headdistance parameter
	if (args.getOptionValue("headdistance").length()) {
		_headDistance = atoi(args.getOptionValue("headdistance").c_str());
	} else {
		_headDistance = 70;
	}

	// --dwelltime parameter
	if (args.getOptionValue("dwelltime").length()) {
		_dwellTime = atoi(args.getOptionValue("dwelltime").c_str());
	} else {
		_dwellTime = 20;
	}

	// --testdwelltime parameter
	if (args.getOptionValue("testdwelltime").length()) {
		_testDwellTime = atoi(args.getOptionValue("testdwelltime").c_str());
	} else {
		_testDwellTime = 20;
	}

	// --sleep parameter
	if (args.getOptionValue("sleep").length()) {
		_sleepTime = atoi(args.getOptionValue("sleep").c_str());
	 } else {
		_sleepTime = 0;
	 }

	_basePath = Utils::getUniqueFileName(outputFolder, subject + "_" + setup + "_" + args.getOptionValue("resolution"));

	_outputFile = new std::ofstream((_basePath + "_").c_str());

	// First write the system time
	time_t currentTime = time(NULL);
	*_outputFile << ctime(&currentTime) << std::endl;

	// Then write the setup parameters
	*_outputFile << "--headdistance=" << _headDistance << std::endl;
	*_outputFile << "--setup=" << setup << std::endl;
	*_outputFile << "--subject=" << subject << std::endl << std::endl;

	// Write the video and screen resolution
	*_outputFile << "Video resolution: " << _videoInput->resolution.height << " x " << _videoInput->resolution.width << std::endl;

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();
	Gdk::Rectangle rect;
	screen->get_monitor_geometry(Gdk::Screen::get_default()->get_n_monitors() - 1, rect);
	*_outputFile << "Screen resolution: " << rect.get_width() << " x " << rect.get_height() << " (Position: "<< rect.get_x() << ", "<< rect.get_y() << ")" << std::endl << std::endl;

	 _outputFile->flush();

	_directory = setup;

	canvas.reset(cvCreateImage(_videoInput->resolution, 8, 3));
	trackingSystem.reset(new TrackingSystem(_videoInput->resolution));

	trackingSystem->gazeTracker.outputFile = _outputFile;

	repositioningImage = cvCreateImage(_videoInput->resolution, 8, 3);

	Application::faceRectangle = NULL;
}

MainGazeTracker::~MainGazeTracker() {
	cleanUp();
}

void MainGazeTracker::process() {
	_totalFrameCount++;
	_frameCount++;

	_videoInput->updateFrame();

	// Wait a little so that the marker stays on the screen for a longer time
	if (Application::status == Application::STATUS_CALIBRATING || Application::status == Application::STATUS_TESTING) {
		usleep(_sleepTime);
	}

	const IplImage *frame = _videoInput->frame;
	canvas->origin = frame->origin;

	double imageNorm = 0.0;

	if (Application::status == Application::STATUS_PAUSED) {
		cvAddWeighted(frame, 0.5, _overlayImage, 0.5, 0.0, canvas.get());

		// Only calculate norm in the area containing the face
		if (_faces.size() == 1) {
			cvSetImageROI(const_cast<IplImage *>(frame), _faces[0]);
			cvSetImageROI(_overlayImage, _faces[0]);

			imageNorm = cvNorm(frame, _overlayImage, CV_L2);
			imageNorm = (10000 * imageNorm) / (_faces[0].width * _faces[0].height);

			// To be able to use the same threshold for VGA and 720 cameras
			if (_videoInput->resolution.height == 720) {
				imageNorm *= 1.05;
			}

			std::cout << "ROI NORM: " << imageNorm << " (" << _faces[0].width << "x" << _faces[0].height << ")" << std::endl;
			cvResetImageROI(const_cast<IplImage *>(frame));
			cvResetImageROI(_overlayImage);
		} else {
			imageNorm = cvNorm(frame, _overlayImage, CV_L2);
			imageNorm = (15000 * imageNorm) / (_videoInput->resolution.height * _videoInput->resolution.height);

			// To be able to use the same threshold for only-face method and all-image method
			imageNorm *= 1.2;

			// To be able to use the same threshold for VGA and 720 cameras
			if (_videoInput->resolution.height == 720) {
				imageNorm *= 1.05;
			}
			//std::cout << "WHOLE NORM: " << imageNorm << std::endl;
		}
	} else {
		cvCopy(frame, canvas.get(), 0);
	}

	try {
		trackingSystem->process(frame, canvas.get());

		if (trackingSystem->gazeTracker.isActive()) {
			if (Application::status != Application::STATUS_TESTING) {
				trackingSystem->gazeTracker.output.setActualTarget(Point(0, 0));
				trackingSystem->gazeTracker.output.setFrameId(0);
			} else {
				trackingSystem->gazeTracker.output.setActualTarget(Point(target->getActivePoint().x, target->getActivePoint().y));
				trackingSystem->gazeTracker.output.setFrameId(target->getPointFrame());
			}

			//trackingSystem->gazeTracker.output.setErrorOutput(Application::status == Application::STATUS_TESTING);	// No longer necessary, TODO REMOVE

			xForEach(iter, _stores) {
				(*iter)->store(trackingSystem->gazeTracker.output);
			}

			// Write the same info to the output text file
			if (_outputFile != NULL) {
				TrackerOutput output = trackingSystem->gazeTracker.output;
				if (Application::status == Application::STATUS_TESTING) {
					std::cout << "TESTING, WRITING OUTPUT!!!!!!!!!!!!!!!!!" << std::endl;
					if (!trackingSystem->eyeExtractor.isBlinking()) {
						*_outputFile << output.frameId + 1 << "\t"
							<< output.actualTarget.x << "\t" << output.actualTarget.y << "\t"
							<< output.gazePoint.x << "\t" << output.gazePoint.y << "\t"
							<< output.gazePointLeft.x << "\t" << output.gazePointLeft.y
							//<< "\t" << output.nnGazePoint.x << "\t" << output.nnGazePoint.y << "\t"
							//<< output.nnGazePointLeft.x << "\t" << output.nnGazePointLeft.y
							<< std::endl;
					} else {
						*_outputFile << output.frameId + 1 << "\t"
							<< output.actualTarget.x << "\t" << output.actualTarget.y << "\t"
							<< 0 << "\t" << 0 << "\t"
							<< 0 << "\t" << 0
							// << "\t" << 0 << "\t" << 0 << "\t"
							//<< 0 << "\t" << 0
							<< std::endl;
					}
				}

				_outputFile->flush();
			}
		}

		//if (!trackingSystem->tracker.areAllPointsActive()) {
		//	throw TrackingException();
		//}
		_frameStoreLoad = 20;
	}
	catch (TrackingException &e) {
		_frameStoreLoad--;
	}

	if (Application::status == Application::STATUS_PAUSED) {
		int rectangleThickness = 15;
		CvScalar color;

		if (imageNorm < 1500) {
			color = CV_RGB(0, 255, 0);
		} else if (imageNorm < 2500) {
			color = CV_RGB(0, 165, 255);
		} else {
			color = CV_RGB(0, 0, 255);
		}

		cvRectangle(canvas.get(), cvPoint(0, 0), cvPoint(rectangleThickness, _videoInput->resolution.height), color, CV_FILLED);	// left
		cvRectangle(canvas.get(), cvPoint(0, 0), cvPoint(_videoInput->resolution.width, rectangleThickness), color, CV_FILLED);	// top
		cvRectangle(canvas.get(), cvPoint(_videoInput->resolution.width - rectangleThickness, 0), cvPoint(_videoInput->resolution.width, _videoInput->resolution.height), color, CV_FILLED);		// right
		cvRectangle(canvas.get(), cvPoint(0, _videoInput->resolution.height - rectangleThickness), cvPoint(_videoInput->resolution.width, _videoInput->resolution.height), color, CV_FILLED);	// bottom

		// Fill the repositioning image so that it can be displayed on the subject's monitor too
		cvAddWeighted(frame, 0.5, _overlayImage, 0.5, 0.0, repositioningImage);

		cvRectangle(repositioningImage, cvPoint(0, 0), cvPoint(rectangleThickness, _videoInput->resolution.height), color, CV_FILLED);	// left
		cvRectangle(repositioningImage, cvPoint(0, 0), cvPoint(_videoInput->resolution.width, rectangleThickness), color, CV_FILLED);		// top
		cvRectangle(repositioningImage, cvPoint(_videoInput->resolution.width - rectangleThickness, 0), cvPoint(_videoInput->resolution.width, _videoInput->resolution.height), color, CV_FILLED);	// right
		cvRectangle(repositioningImage, cvPoint(0, _videoInput->resolution.height - rectangleThickness), cvPoint(_videoInput->resolution.width, _videoInput->resolution.height), color, CV_FILLED);	// bottom
	}

	frameFunctions.process();

	// Show the current target & estimation points on the main window
	if (Application::status == Application::STATUS_CALIBRATING || Application::status == Application::STATUS_TESTING || Application::status == Application::STATUS_CALIBRATED) {
		TrackerOutput output = trackingSystem->gazeTracker.output;
		Point actualTarget(0, 0);
		Point estimation(0, 0);

		if (Application::status == Application::STATUS_TESTING) {
			Utils::mapToVideoCoordinates(target->getActivePoint(), _videoInput->resolution.height, actualTarget, false);
			cvCircle((CvArr *)canvas.get(), cvPoint(actualTarget.x, actualTarget.y), 8, cvScalar(0, 0, 255), -1, 8, 0);
		} else if (Application::status == Application::STATUS_CALIBRATING) {
			Utils::mapToVideoCoordinates(_calibrator->getActivePoint(), _videoInput->resolution.height, actualTarget, false);
			cvCircle((CvArr *)canvas.get(), cvPoint(actualTarget.x, actualTarget.y), 8, cvScalar(0, 0, 255), -1, 8, 0);
		}

		// If not blinking, show the estimation in video
		if (!trackingSystem->eyeExtractor.isBlinking()) {
			Utils::mapToVideoCoordinates(output.gazePoint, _videoInput->resolution.height, estimation, false);
			cvCircle((CvArr *)canvas.get(), cvPoint(estimation.x, estimation.y), 8, cvScalar(0, 255, 0), -1, 8, 0);

			Utils::mapToVideoCoordinates(output.gazePointLeft, _videoInput->resolution.height, estimation, false);
			cvCircle((CvArr *)canvas.get(), cvPoint(estimation.x, estimation.y), 8, cvScalar(255, 0, 0), -1, 8, 0);
		}
	}
}

void MainGazeTracker::cleanUp() {
	_outputFile->close();
	rename((_basePath + "_").c_str(), _basePath.c_str());
}

void MainGazeTracker::startCalibration() {
	Application::status = Application::STATUS_CALIBRATING;

	boost::shared_ptr<WindowPointer> pointer(new WindowPointer(WindowPointer::PointerSpec(30,30,1,0,0.2)));
	calibrationPointer = pointer.get();

	if (Gdk::Screen::get_default()->get_n_monitors() > 1) {
		boost::shared_ptr<WindowPointer> mirror(new WindowPointer(WindowPointer::PointerSpec(30,30,1,0,0)));
		pointer->mirror = mirror;
	}

	std::ifstream calfile((_directory + "/calpoints.txt").c_str());
	boost::shared_ptr<Calibrator> cal(new Calibrator(_frameCount, trackingSystem, scaleByScreen(Calibrator::loadPoints(calfile)), pointer, _dwellTime));
	_calibrator = cal.operator->();

	frameFunctions.clear();
	frameFunctions.addChild(&frameFunctions, cal);
}

void MainGazeTracker::startTesting() {
	Application::status = Application::STATUS_TESTING;

	std::vector<Point> points;
	boost::shared_ptr<WindowPointer> pointer(new WindowPointer(WindowPointer::PointerSpec(30,30,1,0,0.2)));
	calibrationPointer = pointer.get();

	if (Gdk::Screen::get_default()->get_n_monitors() > 1) {
		boost::shared_ptr<WindowPointer> mirror(new WindowPointer(WindowPointer::PointerSpec(30,30,1,0,0)));
		pointer->mirror = mirror;
	}

	// ONUR Modified code to read the test points from a text file
	std::ifstream calfile((_directory + "/testpoints.txt").c_str());
	points = Calibrator::loadPoints(calfile);

	boost::shared_ptr<MovingTarget> moving(new MovingTarget(_frameCount, scaleByScreen(points), pointer, _testDwellTime));

	target = moving.operator->();

	//MovingTarget *target = new MovingTarget(_frameCount, scaleByScreen(points), pointer);
	//shared_ptr<MovingTarget> moving((const boost::shared_ptr<MovingTarget>&) *target);

	*_outputFile << "TESTING" << std::endl << std::endl;

	frameFunctions.clear();
	frameFunctions.addChild(&frameFunctions, moving);
}

void MainGazeTracker::choosePoints() {
	try {
		Point eyes[2];
		Point nose[2];
		Point mouth[2];
		Point eyebrows[2];

		Detection::detectEyeCorners(_videoInput->frame, _videoInput->resolution.height, eyes);

		CvRect noseRect = cvRect(eyes[0].x, eyes[0].y, fabs(eyes[0].x - eyes[1].x), fabs(eyes[0].x - eyes[1].x));
		checkRectSize(_videoInput->frame, &noseRect);
		//std::cout << "Nose rect: " << noseRect.x << ", " << noseRect.y << " - " << noseRect.width << ", " << noseRect.height << std::endl;

		if (!Detection::detectNose(_videoInput->frame, _videoInput->resolution.height, noseRect, nose)) {
			std::cout << "NO NOSE" << std::endl;
			return;
		}

		CvRect mouthRect = cvRect(eyes[0].x, nose[0].y, fabs(eyes[0].x - eyes[1].x), 0.8 * fabs(eyes[0].x - eyes[1].x));
		checkRectSize(_videoInput->frame, &mouthRect);

		if (!Detection::detectMouth(_videoInput->frame, _videoInput->resolution.height, mouthRect, mouth)) {
			std::cout << "NO MOUTH" << std::endl;
			return;
		}

		CvRect eyebrowRect = cvRect(eyes[0].x + fabs(eyes[0].x - eyes[1].x) * 0.25, eyes[0].y - fabs(eyes[0].x - eyes[1].x) * 0.40, fabs(eyes[0].x - eyes[1].x) * 0.5, fabs(eyes[0].x - eyes[1].x) * 0.25);
		checkRectSize(_videoInput->frame, &eyebrowRect);
		Detection::detectEyebrowCorners(_videoInput->frame, _videoInput->resolution.height, eyebrowRect, eyebrows);

		trackingSystem->pointTracker.clearTrackers();
		trackingSystem->pointTracker.addTracker(eyes[0]);
		trackingSystem->pointTracker.addTracker(eyes[1]);
		trackingSystem->pointTracker.addTracker(nose[0]);
		trackingSystem->pointTracker.addTracker(nose[1]);
		trackingSystem->pointTracker.addTracker(mouth[0]);
		trackingSystem->pointTracker.addTracker(mouth[1]);
		trackingSystem->pointTracker.addTracker(eyebrows[0]);
		trackingSystem->pointTracker.addTracker(eyebrows[1]);

		std::cout << "EYES: " << eyes[0] << " + " << eyes[1] << std::endl;
		std::cout << "NOSE: " << nose[0] << " + " << nose[1] << std::endl;
		std::cout << "MOUTH: " << mouth[0] << " + " << mouth[1] << std::endl;
		std::cout << "EYEBROWS: " << eyebrows[0] << " + " << eyebrows[1] << std::endl;

		// Save point selection image
		trackingSystem->pointTracker.saveImage();

		// Calculate the area containing the face
		extractFaceRegionRectangle(_videoInput->frame, trackingSystem->pointTracker.getPoints(&PointTracker::lastPoints, true));
		trackingSystem->pointTracker.normalizeOriginalGrey();
	}
	catch (std::ios_base::failure &e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
}

void MainGazeTracker::clearPoints() {
	trackingSystem->pointTracker.clearTrackers();
}

void MainGazeTracker::pauseOrRepositionHead() {
	if (Application::status == Application::STATUS_PAUSED) {
		Application::status = Application::isTrackerCalibrated ? Application::STATUS_CALIBRATED : Application::STATUS_IDLE;
		trackingSystem->pointTracker.retrack(_videoInput->frame, 2);
	} else {
		Application::status = Application::STATUS_PAUSED;
		_overlayImage = cvLoadImage("point-selection-frame.png", CV_LOAD_IMAGE_COLOR);
		_faces = FaceDetector::faceDetector.detect(_overlayImage);
	}
}

void MainGazeTracker::extractFaceRegionRectangle(IplImage *frame, std::vector<Point> featurePoints) {
	int minX = 10000;
	int maxX = 0;
	int minY = 10000;
	int maxY = 0;

	// Find the boundaries of the feature points
	for (int i = 0; i < (int)featurePoints.size(); i++) {
		minX = featurePoints[i].x < minX ? featurePoints[i].x : minX;
		minY = featurePoints[i].y < minY ? featurePoints[i].y : minY;
		maxX = featurePoints[i].x > maxX ? featurePoints[i].x : maxX;
		maxY = featurePoints[i].y > maxY ? featurePoints[i].y : maxY;
	}

	int diffX = maxX - minX;
	int diffY = maxY - minY;

	minX -= 0.4 * diffX;
	maxX += 0.4 * diffX;
	minY -= 0.5 * diffY;
	maxY += 0.5 * diffY;

	Application::faceRectangle = new CvRect();
	Application::faceRectangle->x = minX;
	Application::faceRectangle->y = minY;
	Application::faceRectangle->width = maxX - minX;
	Application::faceRectangle->height = maxY - minY;
}

