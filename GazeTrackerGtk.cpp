#include "GazeTrackerGtk.h"
#include "Application.h"

GazeTrackerGtk::GazeTrackerGtk():
	_gazeArea(),
	_testArea(&(MainGazeTracker::instance().trackingSystem->gazeTracker.output)),
	_vbox(false, 0),
	_buttonBar(true, 0),
	_calibrateButton("Calibrate"),
	_chooseButton("Choose points"),
	_pauseButton("Pause"),
	_testButton("Test")
{
	try {
		set_title("opengazer 0.1.1");

		Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();
		Gdk::Rectangle rect;
		screen->get_monitor_geometry(Gdk::Screen::get_default()->get_n_monitors() - 1, rect);

		set_size_request(rect.get_width(), rect.get_height());
		move(0, 0);

		// Construct view
		add(_vbox);

		_vbox.pack_start(_buttonBar, false, true, 0);
		_vbox.pack_start(_gazeArea);
		_vbox.pack_start(_testArea);

		_buttonBar.pack_start(_chooseButton);
		_buttonBar.pack_start(_calibrateButton);
		_buttonBar.pack_start(_testButton);
		_buttonBar.pack_start(_pauseButton);

		// Connect buttons
		MainGazeTracker &gazeTracker = MainGazeTracker::instance();
		_calibrateButton.signal_clicked().connect(sigc::mem_fun(gazeTracker, &MainGazeTracker::startCalibration));
		_calibrateButton.signal_clicked().connect(sigc::bind<std::string>(sigc::mem_fun(this, &GazeTrackerGtk::toggleView), "calibrate"));
		_testButton.signal_clicked().connect(sigc::mem_fun(gazeTracker, &MainGazeTracker::startTesting));
		_testButton.signal_clicked().connect(sigc::bind<std::string>(sigc::mem_fun(this, &GazeTrackerGtk::toggleView), "test"));
		_chooseButton.signal_clicked().connect(sigc::mem_fun(gazeTracker, &MainGazeTracker::choosePoints));
		_chooseButton.signal_clicked().connect(sigc::bind<std::string>(sigc::mem_fun(this, &GazeTrackerGtk::toggleView), "choose"));
		_pauseButton.signal_clicked().connect(sigc::mem_fun(gazeTracker, &MainGazeTracker::pauseOrRepositionHead));
		_pauseButton.signal_clicked().connect(sigc::bind<std::string>(sigc::mem_fun(this, &GazeTrackerGtk::toggleView), "pause"));
		_pauseButton.signal_clicked().connect(sigc::mem_fun(this, &GazeTrackerGtk::changePauseButtonText));

		// Display view
		_vbox.show();
		_buttonBar.show();
		_gazeArea.show();
		_testArea.hide();
		_calibrateButton.show();
		_chooseButton.show();
		_pauseButton.show();
		_testButton.show();
	}
	catch (Utils::QuitNow) {
		std::cout << "Caught it!\n";
	}
}

GazeTrackerGtk::~GazeTrackerGtk() {}

void GazeTrackerGtk::changePauseButtonText() {
	if(_pauseButton.get_label() == "Pause") {
		_pauseButton.set_label("Unpause");
	} else {
		_pauseButton.set_label("Pause");
	}
}

void GazeTrackerGtk::toggleView(std::string button) {
	if (button == "choose" || button == "pause") {
		_gazeArea.show();
		_testArea.hide();
	} else {
		_gazeArea.hide();
		_testArea.show();
	}
}

