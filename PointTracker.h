#pragma once

#include <boost/scoped_ptr.hpp>

#include "Point.h"

class PointTracker {
public:
	static const int eyePoint1 = 0;
	static const int eyePoint2 = 1;
	std::vector<uchar> status;
	std::vector<cv::Point2f> origPoints, currentPoints, lastPoints;

	PointTracker(const cv::Size &size);
	
	void clearTrackers();
	void addTracker(const Point &point);
	void updateTracker(int id, const Point &point);
	void removeTracker(int id);
	int getClosestTracker(const Point &point);
	
	void process();
	void track();
	void retrack();
	int countActivePoints();
	bool areAllPointsActive();
	bool isTrackingSuccessful();
	int pointCount();
	std::vector<Point> getPoints(const std::vector<cv::Point2f> PointTracker::*points, bool allPoints=true);
	void draw();
	void normalizeOriginalGrey();

	void saveImage();

private:
	static const int _winSize = 11;
	int _flags;
	cv::Mat _grey, _origGrey, _lastGrey;
	static const int _pyramidDepth = 2;
	
	void synchronizePoints();
};

