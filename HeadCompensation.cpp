#include "HeadCompensation.h"
#include "Application.h"

HeadCompensation::HeadCompensation():
		_xParams(3),
		_yParams(3),
		_xa0(0.0), _xa1(0.0), _xa2(0.0),
		_ya0(0.0), _ya1(0.0), _ya2(0.0),
		_samples(0)
{
}

void HeadCompensation::addCorrection(Point correction) {
	_xParams.addSample(Application::Components::headTracker->rotX, Application::Components::headTracker->rotY, 1.0, correction.x);
	_yParams.addSample(Application::Components::headTracker->rotX, Application::Components::headTracker->rotY, 1.0, correction.y);
	_samples++;
}

void HeadCompensation::updateFactors(void) {
	if (_samples > 0) {
		_xParams.solve(_xa0, _xa1, _xa2);
		_yParams.solve(_ya0, _ya1, _ya2);
	}
}

Point HeadCompensation::estimateCorrection(void) {
	return Point(_xa0 * Application::Components::headTracker->rotX + _xa1 * Application::Components::headTracker->rotY + _xa2, _ya0 * Application::Components::headTracker->rotX + _ya1 * Application::Components::headTracker->rotY + _ya2);
}