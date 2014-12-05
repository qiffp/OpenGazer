#include "GazeTracker.h"
#include "WindowPointer.h"
#include "MainGazeTracker.h"

class TestArea: public Gtk::DrawingArea {
public:
	TestArea(TrackerOutput *output);
	virtual ~TestArea();
	void showContents();
	void clearLastUpdatedRegion();
	void displayImageCentered(IplImage *image);

private:
	TrackerOutput *_trackerOutput;
	CvSize _areaSize;
	IplImage *_target;
	IplImage *_clearingImage;
	CvRect _lastUpdatedRegion;
	bool _isWindowInitialized;

	bool onIdle();

	// Gtk::DrawingArea;
	bool on_expose_event(GdkEventExpose *event);
	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
};

