#include "GazeTracker.h"
#include "WindowPointer.h"
#include "MainGazeTracker.h"

class TestArea: public Gtk::DrawingArea {
public:
	TestArea(TrackerOutput *output);
	virtual ~TestArea();
	void showContents();
	void clearLastUpdatedRegion();

private:
	TrackerOutput *_trackerOutput;
	CvSize _areaSize;
	IplImage *_target;
	IplImage *_clearingImage;
	CvRect _lastUpdatedRegion;

	bool onIdle();
};

