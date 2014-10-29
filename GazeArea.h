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
	bool on_expose_event(GdkEventExpose *event);
	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
};
