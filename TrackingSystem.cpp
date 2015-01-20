#include "TrackingSystem.h"
#include "FeatureDetector.h"
#include "BlinkDetector.h"
#include "Application.h"

TrackingSystem::TrackingSystem(CvSize size):
	pointTracker(size),
	eyeExtractor(pointTracker),
	_headTracker(pointTracker),
	_headCompensation(_headTracker)
{
}

void TrackingSystem::process(const IplImage *frame, IplImage *image) {
	if (Application::status != Application::STATUS_PAUSED) {
		pointTracker.track(frame, 2);

		if (pointTracker.countActivePoints() < 4) {
			pointTracker.draw(image);
			throw TrackingException();
		}

		_headTracker.updateTracker();
		eyeExtractor.extractEyes(frame);	// throws Tracking Exception
		gazeTracker.update(eyeExtractor.eyeFloat.get(), eyeExtractor.eyeGrey.get());
		gazeTracker.updateLeft(eyeExtractor.eyeFloatLeft.get(), eyeExtractor.eyeGreyLeft.get());

		displayEye(image, 0, 0, 0, 2);
		pointTracker.draw(image);
		_headTracker.draw(image);
	}
}

void TrackingSystem::displayEye(IplImage *image, int baseX, int baseY, int stepX, int stepY) {
	CvSize eyeSize = EyeExtractor::eyeSize;
	int eyeDX = EyeExtractor::eyeDX;
	int eyeDY = EyeExtractor::eyeDY;

	static IplImage *eyeGreyTemp = cvCreateImage(eyeSize, 8, 1);
	static FeatureDetector features(eyeSize);
	static FeatureDetector features_left(eyeSize);

	features.addSample(eyeExtractor.eyeGrey.get());

	baseX *= 2 * eyeDX;
	baseY *= 2 * eyeDY;
	stepX *= 2 * eyeDX;
	stepY *= 2 * eyeDY;

	cvSetImageROI(image, cvRect(baseX, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyeGrey.get(), image, CV_GRAY2RGB);

	cvSetImageROI(image, cvRect(baseX + stepX * 1, baseY + stepY * 1, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyeGrey.get(), image, CV_GRAY2RGB);

	cvConvertScale(features.getMean().get(), eyeGreyTemp);
	cvSetImageROI(image, cvRect(baseX, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	// ONUR DUPLICATED CODE FOR LEFT EYE
	features_left.addSample(eyeExtractor.eyeGreyLeft.get());

	cvSetImageROI(image, cvRect(baseX + 100, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyeGreyLeft.get(), image, CV_GRAY2RGB);

	cvSetImageROI(image, cvRect(baseX + 100, baseY + stepY * 1, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyeGreyLeft.get(), image, CV_GRAY2RGB);

	cvConvertScale(features_left.getMean().get(), eyeGreyTemp);
	cvSetImageROI(image, cvRect(baseX + 100, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	cvResetImageROI(image);
}
