#pragma once

#include "LeastSquares.h"
#include "HeadTracker.h"

class HeadCompensation {
public:
	HeadCompensation();

	void addCorrection(Point correction);
	void updateFactors(void);
	Point estimateCorrection(void);

private:
	LeastSquares _xParams;
	LeastSquares _yParams;
	double _xa0, _xa1, _xa2;
	double _ya0, _ya1, _ya2;
	int _samples;
};
