#pragma once

#include "GazeArea.h"
#include "TestArea.h"

class GazeTrackerGtk: public Gtk::Window {
public:
	GazeTrackerGtk();
	virtual ~GazeTrackerGtk();

private:
	GazeArea _gazeArea;
	TestArea _testArea;
	Gtk::VBox _vbox;
	Gtk::HBox _buttonBar;
	Gtk::Button _calibrateButton;
	Gtk::Button _chooseButton;
	Gtk::Button _pauseButton;
	Gtk::Button _testButton;

	void changePauseButtonText();
	void toggleView(std::string button);
};
