#pragma once

#include <boost/shared_ptr.hpp>

#include "MainGazeTracker.h"
#include "OutputMethods.h"

class GazeArea: public Gtk::DrawingArea {
public:
	GazeArea();
	virtual ~GazeArea();

private:
	int _lastPointId;
	int _clickCount;

	bool onIdle();

	// Gtk::DrawingArea
	virtual bool on_expose_event(GdkEventExpose *event);
	virtual bool on_button_press_event(GdkEventButton *event);
	virtual bool on_button_release_event(GdkEventButton *event);
};
