#include <opencv/highgui.h>
#include <math.h>

#include "Detection.h"
#include "Application.h"
#include "utils.h"

namespace {
	std::vector<cv::Point2f>* detectCornersInGrayscale(cv::Mat eyeRegionImageGray, int cornerCount) {
		std::vector<cv::Point2f> *corners = new std::vector<cv::Point2f>();
		
		double qualityLevel = 0.01;
		double minDistance = 2;
		int eigBlockSize = 3;
		int useHarris = false;

		cv::goodFeaturesToTrack(eyeRegionImageGray, *corners, cornerCount, qualityLevel, minDistance, cv::noArray(), eigBlockSize, useHarris);

		return corners;
	}

	void checkRectSize(const cv::Mat &image, cv::Rect *rect) {
		if (rect->x < 0) {
			rect->x = 0;
		}

		if (rect->y < 0) {
			rect->y = 0;
		}

		if (rect->x + rect->width >= image.size().width) {
			rect->width = image.size().width - rect->x - 1;
		}

		if (rect->y + rect->height >= image.size().height) {
			rect->height = image.size().height - rect->y - 1;
		}
	}
}

namespace Detection {
	cv::CascadeClassifier faceCascade;
	cv::CascadeClassifier eyeCascade;
	cv::CascadeClassifier noseCascade;
	cv::CascadeClassifier mouthCascade;


	void choosePoints() {
		try {
			Point eyes[2];
			Point nose[2];
			Point mouth[2];
			Point eyebrows[2];

			std::cout << "Detecting eye corners" << std::endl;
			
			if(!Detection::detectEyeCorners(Application::Components::videoInput->frame, Application::Components::videoInput->getResolution(), eyes)) {
				std::cout << "EYE CORNERS NOT DETECTED" << std::endl;
				return;
			}

			std::cout << "Detecting nose corners" << std::endl;
			cv::Rect noseRect = cv::Rect(eyes[0].x, eyes[0].y, fabs(eyes[0].x - eyes[1].x), fabs(eyes[0].x - eyes[1].x));
			checkRectSize(Application::Components::videoInput->frame, &noseRect);
			std::cout << "Nose rect: " << noseRect.x << ", " << noseRect.y << " - " << noseRect.width << ", " << noseRect.height << std::endl;
			
			if (!Detection::detectNose(Application::Components::videoInput->frame, Application::Components::videoInput->getResolution(), noseRect, nose)) {
				std::cout << "NO NOSE" << std::endl;
				return;
			}

			std::cout << "Detecting mouth corners" << std::endl;
			cv::Rect mouthRect = cv::Rect(eyes[0].x, nose[0].y, fabs(eyes[0].x - eyes[1].x), 0.8 * fabs(eyes[0].x - eyes[1].x));
			checkRectSize(Application::Components::videoInput->frame, &mouthRect);

			if (!Detection::detectMouth(Application::Components::videoInput->frame, Application::Components::videoInput->getResolution(), mouthRect, mouth)) {
				std::cout << "NO MOUTH" << std::endl;
				return;
			}

			std::cout << "Detecting eyebrow corners" << std::endl;
			cv::Rect eyebrowRect = cv::Rect(eyes[0].x + fabs(eyes[0].x - eyes[1].x) * 0.25, eyes[0].y - fabs(eyes[0].x - eyes[1].x) * 0.40, fabs(eyes[0].x - eyes[1].x) * 0.5, fabs(eyes[0].x - eyes[1].x) * 0.25);
			checkRectSize(Application::Components::videoInput->frame, &eyebrowRect);
			Detection::detectEyebrowCorners(Application::Components::videoInput->frame, Application::Components::videoInput->getResolution(), eyebrowRect, eyebrows);

			//cvSaveImage("cframe.jpg", Application::Components::videoInput->frame);

			std::cout << "Adding trackers" << std::endl;
			Application::Components::pointTracker->clearTrackers();

			Application::Components::pointTracker->addTracker(eyes[0]);
			Application::Components::pointTracker->addTracker(eyes[1]);
			Application::Components::pointTracker->addTracker(nose[0]);
			Application::Components::pointTracker->addTracker(nose[1]);
			Application::Components::pointTracker->addTracker(mouth[0]);
			Application::Components::pointTracker->addTracker(mouth[1]);
			Application::Components::pointTracker->addTracker(eyebrows[0]);
			Application::Components::pointTracker->addTracker(eyebrows[1]);

			std::cout << "EYES: " << eyes[0] << " + " << eyes[1] << std::endl;
			std::cout << "NOSE: " << nose[0] << " + " << nose[1] << std::endl;
			std::cout << "MOUTH: " << mouth[0] << " + " << mouth[1] << std::endl;
			std::cout << "EYEBROWS: " << eyebrows[0] << " + " << eyebrows[1] << std::endl;

			// Save point selection image
			Application::Components::pointTracker->saveImage();

			// Calculate the area containing the face
			//extractFaceRegionRectangle(Application::Components::pointTracker->getPoints(&PointTracker::lastPoints, true));
			//Application::Components::pointTracker->normalizeOriginalGrey();
		}
		catch (std::ios_base::failure &e) {
			std::cout << e.what() << std::endl;
		}
		catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		}
	}
	
	void loadCascades() {
		if(/*!faceCascade.load("haarcascade_frontalface_alt.xml") || */
			!eyeCascade.load("DetectorEyes.xml") || 
			!noseCascade.load("DetectorNose2.xml") || 
			!mouthCascade.load("DetectorMouth.xml")) {
	        std::cout << "ERROR: Could not load cascade classifiers!" << std::endl;
			exit(1);
		}
		
		std::cout << "Detection cascades loaded!" << std::endl;
	}
	
	bool detectLargestObject(cv::CascadeClassifier cascade, cv::Mat image, cv::Rect &largestObject, double scaleFactor, int minNeighbors, int flags, cv::Size minSize) {
		std::vector<cv::Rect> results;
		
		largestObject.x = 0;
		largestObject.y = 0;
		largestObject.width = 0;
		largestObject.height = 0;
		
		cascade.detectMultiScale(image, results, scaleFactor, minNeighbors, flags, minSize);
		
		
		std::cout << "detectLargestObject(): results size: " << results.size() << std::endl;
		
		// Save the largest object
		if (results.size() > 0) {
	    	for(int i=0; i<results.size(); i++) {
	    		cv::Rect temp = results[0];

	    		if((temp.width * temp.height) > (largestObject.width*largestObject.height)) {
	    			largestObject = temp;
				}
	    	}
	
			return true;
		}
		
		return false;
	}
	
	bool detectNose(cv::Mat image, double resolution, cv::Rect noseRect, Point points[]) {
    	cv::Rect largestObject(0, 0, 0, 0);
		double scaleFactor = 1.1;
		int minNeighbors = 3;
		int flags = CV_HAAR_DO_CANNY_PRUNING;
		cv::Size minSize(24, 20);
		
		if (resolution != 480) {
			double factor = resolution/480;
			minSize.width = round(factor*minSize.width);
			minSize.height = round(factor*minSize.height);
		}
		
		// Detect objects
		if(!detectLargestObject(noseCascade, image(noseRect), largestObject, scaleFactor, minNeighbors, CV_HAAR_DO_CANNY_PRUNING, minSize)) {
			return false;
		}
		
		points[0] = Point(noseRect.x + largestObject.x + largestObject.width * 0.33, 
							noseRect.y + largestObject.y + largestObject.height * 0.6);
		points[1] = Point(noseRect.x + largestObject.x + largestObject.width * 0.67, 
							noseRect.y + largestObject.y + largestObject.height * 0.6);

		//cv::Rectangle(image, cv::Point(noseRect.x + nose->x, noseRect.y + nose->y), cv::Point(noseRect.x + nose->x + nose->width, noseRect.y + nose->y + nose->height), CV_RGB(0, 255, 0), 2, 8, 0);
		//cv::circle(image, cv::Point(points[0].x, points[0].y), 3, CV_RGB(0,255,0), -1, 8, 0);
		//cv::circle(image, cv::Point(points[1].x, points[1].y), 3, CV_RGB(0,255,0), -1, 8, 0);

		return true;
	}

	bool detectMouth(cv::Mat image, double resolution, cv::Rect mouthRect, Point points[]) {
		cv::Rect largestObject(0, 0, 0, 0);
		double scaleFactor = 1.1;
		int minNeighbors = 3;
		int flags = 0;
		cv::Size minSize(25, 15);
		
		if (resolution != 480) {
			double factor = resolution/480;
			minSize.width = round(factor*minSize.width);
			minSize.height = round(factor*minSize.height);
		}
		
		// Detect objects
		if(!detectLargestObject(mouthCascade, image(mouthRect), largestObject, scaleFactor, minNeighbors, CV_HAAR_DO_CANNY_PRUNING, minSize)) {
			return false;
		}

		points[0] = Point(mouthRect.x + largestObject.x + largestObject.width * 0.1, 
							mouthRect.y + largestObject.y + largestObject.height * 0.4);
		points[1] = Point(mouthRect.x + largestObject.x + largestObject.width * 0.9, 
							mouthRect.y + largestObject.y + largestObject.height * 0.4);

		//cv::Rectangle(image, cv::Point(mouthRect.x + largestObject.x, mouthRect.y + largestObject.y), cv::Point(mouthRect.x + largestObject.x + largestObject.width, mouthRect.y + largestObject.y + largestObject.height), CV_RGB(0, 255, 0), 2, 8, 0 );
		//cv::circle(image, cv::Point(points[0].x, points[0].y), 3, CV_RGB(0,255,0), -1, 8, 0);
		//cv::circle(image, cv::Point(points[1].x, points[1].y), 3, CV_RGB(0,255,0), -1, 8, 0);

		return true;
	}

	bool detectEyeCorners(cv::Mat image, double resolution, Point points[]) {
		cv::Rect largestObject(0, 0, 0, 0);
		double scaleFactor = 1.1;
		int minNeighbors = 10;
		int flags = CV_HAAR_DO_CANNY_PRUNING;
		cv::Size minSize(64, 16);
		
		if (resolution != 480) {
			double factor = resolution/480;
			minSize.width = round(factor*minSize.width);
			minSize.height = round(factor*minSize.height);
		}
		
		// Detect objects
		if(!detectLargestObject(eyeCascade, image, largestObject, scaleFactor, minNeighbors, CV_HAAR_DO_CANNY_PRUNING, minSize)) {
			std::cout << "Detect largest object (for eyes) failed!!" << std::endl;
			return false;
		}

		std::cout << "Resolution: " << resolution << ", both eye reg.:" << largestObject.width << ", " << largestObject.height << std::endl;

		//cv::Rectangle(image, cv::Point(largestObject.x, largestObject.y), cv::Point(largestObject.x + largestObject.width, largestObject.y + largestObject.height), CV_RGB(0, 255, 0), 2, 8, 0);

		int cornerCount = 100;
		cv::Mat eyeRegionImage(cv::Size(largestObject.width, largestObject.height), CV_8UC3);
		cv::Mat eyeRegionImageGray(cv::Size(largestObject.width, largestObject.height), CV_8UC1);
		
		image(largestObject).copyTo(eyeRegionImage);
		cv::cvtColor(eyeRegionImage, eyeRegionImageGray, CV_RGB2GRAY);

		Utils::normalizeGrayScaleImage(&eyeRegionImageGray, 127, 80);

		std::vector<cv::Point2f> *corners = detectCornersInGrayscale(eyeRegionImageGray, cornerCount);

		int leftEyeCornersXSum = 0;
		int leftEyeCornersYSum = 0;
		int leftEyeCornersCount = 0;

		int rightEyeCornersXSum = 0;
		int rightEyeCornersYSum = 0;
		int rightEyeCornersCount = 0;

		/// Drawing a circle around corners
		for (int j = 0; j < corners->size(); j++ ) {
			if ((*corners)[j].x < largestObject.width * 0.4) {
				leftEyeCornersXSum += (*corners)[j].x;
				leftEyeCornersYSum += (*corners)[j].y;
				leftEyeCornersCount++;
				//cv::circle(eyeRegionImage, cv::Point(corners[j].x, corners[j].y), 3, CV_RGB(255,0,0), -1, 8,0);
			} else if ((*corners)[j].x > largestObject.width * 0.6) {
				rightEyeCornersXSum += (*corners)[j].x;
				rightEyeCornersYSum += (*corners)[j].y;
				rightEyeCornersCount++;
				//cv::circle(eyeRegionImage, cv::Point(corners[j].x, corners[j].y), 3, CV_RGB(255,0,0), -1, 8,0);
			}
		}

		double leftEyeCenterX = largestObject.x + (leftEyeCornersXSum / (double)leftEyeCornersCount);
		double leftEyeCenterY = largestObject.y + (leftEyeCornersYSum / (double)leftEyeCornersCount);

		double rightEyeCenterX = largestObject.x + (rightEyeCornersXSum / (double)rightEyeCornersCount);
		double rightEyeCenterY = largestObject.y + (rightEyeCornersYSum / (double)rightEyeCornersCount);

		double xDiff = rightEyeCenterX - leftEyeCenterX;
		double yDiff = rightEyeCenterY - leftEyeCenterY;
		
		points[0] = Point(leftEyeCenterX - 0.29 * xDiff, leftEyeCenterY - 0.29 * yDiff);// + xDiff/40);
		points[1] = Point(rightEyeCenterX + 0.29 * xDiff, rightEyeCenterY + 0.29 * yDiff);// + xDiff/40);

		/// Drawing a circle around corners
		//for (int i = 0; i < cornerCount; i++) {
		//	cv::circle(eyeRegionImage, cv::Point(corners[i].x, corners[i].y), 3, CV_RGB(255,0,0), -1, 8, 0);
		//}

		//cv::circle(image, cv::Point(points[0].x, points[0].y), 3, CV_RGB(0,255,0), -1, 8, 0);
		//cv::circle(image, cv::Point(points[1].x, points[1].y), 3, CV_RGB(0,255,0), -1, 8, 0);
		return true;
	}

	void detectEyebrowCorners(cv::Mat image, double resolution, cv::Rect eyebrowRect, Point points[]) {
		eyebrowRect.width = eyebrowRect.width / 2;
		
		cv::Mat eyebrowRegionImage(cv::Size(eyebrowRect.width, eyebrowRect.height), CV_8UC3);
		cv::Mat eyebrowRegionImageGray(cv::Size(eyebrowRect.width, eyebrowRect.height), CV_8UC1);
		cv::Mat eyebrowRegionImage2(cv::Size(eyebrowRect.width, eyebrowRect.height), CV_8UC3);
		cv::Mat eyebrowRegionImageGray2(cv::Size(eyebrowRect.width, eyebrowRect.height), CV_8UC1);

		std::cout << "EYEBROW x, y = " << eyebrowRect.x << " - " << eyebrowRect.y << " width, height =" << eyebrowRect.width << " - " << eyebrowRect.height << std::endl;

		image(eyebrowRect).copyTo(eyebrowRegionImage);
		
		cv::Rect eyebrowRect2 = cv::Rect(eyebrowRect.x + eyebrowRect.width, eyebrowRect.y, eyebrowRect.width, eyebrowRect.height);
		image(eyebrowRect2).copyTo(eyebrowRegionImage2);
		
		//cvSaveImage("eyebrows.png", eyebrowRegionImage);
		cv::cvtColor(eyebrowRegionImage, eyebrowRegionImageGray, CV_RGB2GRAY);
		cv::cvtColor(eyebrowRegionImage2, eyebrowRegionImageGray2, CV_RGB2GRAY);

		std::vector<cv::Point2f> *corners = detectCornersInGrayscale(eyebrowRegionImageGray, 1);
		std::vector<cv::Point2f> *corners2 = detectCornersInGrayscale(eyebrowRegionImageGray2, 1);

		points[0] = Point(eyebrowRect.x + (*corners)[0].x, eyebrowRect.y + (*corners)[0].y);
		points[1] = Point(eyebrowRect2.x + (*corners2)[0].x, eyebrowRect2.y + (*corners2)[0].y);
	}
}
