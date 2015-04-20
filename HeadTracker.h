#pragma once

#include "PointTracker.h"

class HeadTracker {
public:
	double rotX;
	double rotY;
	double atX;
	double atY;

	HeadTracker();
	void draw();
	void process();

private:
	std::vector<double> _depths;

	std::vector<bool> detectInliers(std::vector<Point> const &prev, std::vector<Point> const &now, double radius=30.0);
	void predictPoints(double xx0, double yy0, double xx1, double yy1, double rotX, double rotY, double atX, double atY);
};
