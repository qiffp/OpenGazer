#include <opencv2/highgui/highgui.hpp>

#include "EyeExtractor.h"
#include "Application.h"
#include "utils.h"

const int EyeExtractor::eyeDX = 64;
const int EyeExtractor::eyeDY = 32;
const cv::Size EyeExtractor::eyeSize = cv::Size(eyeDX * 2, eyeDY * 2);

EyeExtractor::EyeExtractor():
	eyeGrey(new cv::Mat(eyeSize, CV_8UC1)),
	eyeFloat(new cv::Mat(eyeSize, CV_32FC1)),
	
 	// ONUR DUPLICATED CODE FOR LEFT EYE
	eyeGreyLeft(new cv::Mat(eyeSize, CV_8UC1)),
	eyeFloatLeft(new cv::Mat(eyeSize, CV_32FC1)),
	
	eyeImage(new cv::Mat(eyeSize, CV_8UC3)),
	eyeImageLeft(new cv::Mat(eyeSize, CV_8UC3)),

	_isBlinking(false)
{
}

EyeExtractor::~EyeExtractor() {}

void EyeExtractor::process() {
	if(Application::Signals::initiateCalibrationFrameNo == Application::Components::videoInput->frameCount) {
		start();
	}

	if (Application::Components::pointTracker->isTrackingSuccessful()) {
		// Extract eye images using point tracker results
		extractEye(Application::Components::videoInput->frame);
		extractEyeLeft(Application::Components::videoInput->frame);

		// Blink detection
		_blinkDetector.update(eyeFloat);
		_blinkDetectorLeft.update(eyeFloatLeft);

		if (_blinkDetector.getState() >= 2 && _blinkDetectorLeft.getState() >= 2) {
			_isBlinking = true;
		} else {
			_isBlinking = false;
		}
	
		// If calibration is active, collect eye image samples, calculate averages and train the system
		if(Application::Components::calibrator->isActive()) {
			if (Application::Components::calibrator->shouldStartNextPoint()) {
				// Switch to next calibration point
				pointStart();
			} else if (hasValidSample()) {
				// Add the valid training samples
				averageEye->addSample(eyeFloat.get());
				averageEyeLeft->addSample(eyeFloatLeft.get());

				// TODO MOVE ADD SAMPLES TO NN CODE
				//Application::Components::gazeTracker->addSampleToNN(Application::Components::calibrator->getActivePoint(), eyeFloat.get(), eyeGrey.get());
				//Application::Components::gazeTracker->addSampleToNNLeft(Application::Components::calibrator->getActivePoint(), eyeFloatLeft.get(), eyeGreyLeft.get());

				if (Application::Components::calibrator->getPointFrameNo() == Application::dwelltimeParameter - 1) {
					pointEnd();
				
					// TODO : NOT NECESSARY FOR THIS COMPONENT
					//if (Application::Components::calibrator->isLastPoint())
					//	calibrationEnded();
				}
			}
		}
	}
}

bool EyeExtractor::isBlinking() {
	return _isBlinking;
}

void EyeExtractor::extractEye(const cv::Mat originalImage) {
	double x0 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint1].x;
	double y0 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint1].y;
	double x1 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint2].x;
	double y1 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint2].y;

	// Move the tracked points a little towards center (using weighted sum)
	// so that the extracted image contains more the important area (iris & sclera)
	double alpha = 0.95;

	double temp_x0 = alpha*x0 + (1-alpha)*x1;
	double temp_y0 = alpha*y0 + (1-alpha)*y1;

	x1 = (1-alpha)*x0 + alpha*x1;
	y1 = (1-alpha)*y0 + alpha*y1;

	x0 = temp_x0;
	y0 = temp_y0;

	
	double dx = abs(x1 - x0);	// Horizontal distance between eye corners in image
	double dy = 0.19 * dx;		// %20 percent of horizontal distance (used as height of extracted eyes)

	// Calculate the roll angle of the head (using eye corner positions)
	double rollAngle = atan((y1-y0)/(x1 - x0));

	cv::Point2f originalImagePoints[3];
	cv::Point2f extractedImagePoints[3];

	// Top left pixel
	originalImagePoints[0].x = x0 + sin(rollAngle)*dy/2;
	originalImagePoints[0].y = y0 - cos(rollAngle)*dy/2;

	extractedImagePoints[0].x = 0;
	extractedImagePoints[0].y = 0;

	// Bottom left pixel
	originalImagePoints[1].x = x0 - sin(rollAngle)*dy/2;
	originalImagePoints[1].y = y0 + cos(rollAngle)*dy/2;

	extractedImagePoints[1].x = 0;
	extractedImagePoints[1].y = eyeSize.height-1;
	
	// Top right pixel
	originalImagePoints[2].x = originalImagePoints[0].x + cos(rollAngle)*dy*2;
	originalImagePoints[2].y = originalImagePoints[0].y + sin(rollAngle)*dy*2;

	extractedImagePoints[2].x = eyeSize.width-1;
	extractedImagePoints[2].y = 0;

	// Calculate the transformation matrix between two sets of points, and use it to extract eye image
	cv::Mat transform = cv::getAffineTransform(originalImagePoints, extractedImagePoints);
	warpAffine(originalImage, *eyeImage.get(), transform, eyeSize);
	cv::cvtColor(*eyeImage.get(), *eyeGrey.get(), CV_RGB2GRAY);
	
	// Apply blurring and normalization
	//Utils::normalizeGrayScaleImage(eyeGrey.get(), 127, 50);	// TODO ONUR UNCOMMENT
	eyeGrey->convertTo(*eyeFloat, CV_32FC1);
	cv::GaussianBlur(*eyeFloat, *eyeFloat, cv::Size(3,3), 0);
	cv::equalizeHist(*eyeGrey, *eyeGrey);
}

void EyeExtractor::extractEyeLeft(const cv::Mat originalImage) {
	double x0 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint2].x;
	double y0 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint2].y;
	double x1 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint1].x;
	double y1 = Application::Components::pointTracker->currentPoints[PointTracker::eyePoint1].y;

	// Move the tracked points a little towards center (using weighted sum)
	// so that the extracted image contains more the important area (iris & sclera)
	double alpha = 0.95;

	double temp_x0 = alpha*x0 + (1-alpha)*x1;
	double temp_y0 = alpha*y0 + (1-alpha)*y1;

	x1 = (1-alpha)*x0 + alpha*x1;
	y1 = (1-alpha)*y0 + alpha*y1;

	x0 = temp_x0;
	y0 = temp_y0;


	double dx = abs(x1 - x0);	// Horizontal distance between eye corners in image
	double dy = 0.19 * dx;		// %20 percent of horizontal distance (used as height of extracted eyes)

	// Calculate the roll angle of the head (using eye corner positions)
	double rollAngle = atan((y1-y0)/(x1 - x0));

	cv::Point2f originalImagePoints[3];
	cv::Point2f extractedImagePoints[3];

	// Top right pixel
	originalImagePoints[0].x = x0 + sin(rollAngle)*dy/2;
	originalImagePoints[0].y = y0 - cos(rollAngle)*dy/2;

	extractedImagePoints[0].x = eyeSize.width-1;
	extractedImagePoints[0].y = 0;

	// Bottom right pixel
	originalImagePoints[1].x = x0 - sin(rollAngle)*dy/2;
	originalImagePoints[1].y = y0 + cos(rollAngle)*dy/2;

	extractedImagePoints[1].x = eyeSize.width-1;
	extractedImagePoints[1].y = eyeSize.height-1;
	
	// Top left pixel
	originalImagePoints[2].x = originalImagePoints[0].x - cos(rollAngle)*dy*2;
	originalImagePoints[2].y = originalImagePoints[0].y - sin(rollAngle)*dy*2;

	extractedImagePoints[2].x = 0;
	extractedImagePoints[2].y = 0;

	// Calculate the transformation matrix between two sets of points, and use it to extract eye image
	cv::Mat transform = cv::getAffineTransform(originalImagePoints, extractedImagePoints);
	warpAffine(originalImage, *eyeImageLeft.get(), transform, eyeSize);
	cv::cvtColor(*eyeImageLeft.get(), *eyeGreyLeft.get(), CV_RGB2GRAY);
	
	// Apply blurring and normalization
	//Utils::normalizeGrayScaleImage(eyeGreyLeft.get(), 127, 50);	// TODO ONUR UNCOMMENT
	eyeGreyLeft->convertTo(*eyeFloatLeft, CV_32FC1);
	cv::GaussianBlur(*eyeFloatLeft, *eyeFloatLeft, cv::Size(3,3), 0);
	cv::equalizeHist(*eyeGreyLeft, *eyeGreyLeft);
}

void EyeExtractor::draw() {
	if (!Application::Components::pointTracker->isTrackingSuccessful())
		return;
	
	cv::Mat image = Application::Components::videoInput->debugFrame;
	int eyeDX = eyeSize.width;
	int eyeDY = eyeSize.height;

	int baseX = 0;
	int baseY = 0;
	int stepX = 0;
	int stepY = eyeDY;

	cv::cvtColor(*eyeGrey.get(), image(cv::Rect(baseX, baseY, eyeDX, eyeDY)), CV_GRAY2RGB);
	//cv::cvtColor(*eyeGrey.get(), image(cv::Rect(baseX + stepX * 1, baseY + stepY * 1, eyeDX, eyeDY)), CV_GRAY2RGB);
	
	cv::cvtColor(*eyeGreyLeft.get(), image(cv::Rect(baseX + 140, baseY, eyeDX, eyeDY)), CV_GRAY2RGB);
	//cv::cvtColor(*eyeGreyLeft.get(), image(cv::Rect(baseX + 100, baseY + stepY * 1, eyeDX, eyeDY)), CV_GRAY2RGB);
}

// Prepares the eye extractor for calibration
void EyeExtractor::start() {
	pointStart();
}

// When a new calibration point is starting, prepare the average eye accumulators
void EyeExtractor::pointStart() {
	averageEye.reset(new FeatureDetector(EyeExtractor::eyeSize));
	averageEyeLeft.reset(new FeatureDetector(EyeExtractor::eyeSize));
}

void EyeExtractor::pointEnd() {}
void EyeExtractor::abortCalibration() {}
void EyeExtractor::calibrationEnded() {}

bool EyeExtractor::hasValidSample() {
	return Application::Components::calibrator->getPointFrameNo() >= 11 && !isBlinking();
}
