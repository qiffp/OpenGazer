#include <opencv/highgui.h>

#include "TestArea.h"
#include "Application.h"
#include "utils.h"

TestArea::TestArea(TrackerOutput *output):
	_trackerOutput(output)
{
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();
	Gdk::Rectangle rect;
	screen->get_monitor_geometry(Gdk::Screen::get_default()->get_n_monitors() - 1, rect);
	set_size_request(rect.get_width(), rect.get_height());

	Glib::signal_idle().connect(sigc::mem_fun(*this, &TestArea::onIdle));

	_areaSize = cvSize(rect.get_width(), rect.get_height());

	_target = (IplImage *)cvCreateImage(cvSize(50, 50), 8, 3);
	_target = (IplImage *)cvLoadImage("./target.png");

	// Clearing image is filled with white
	_clearingImage = (IplImage *)cvCreateImage(cvSize(2000, 1500), 8, 3);
	cvSet(_clearingImage, CV_RGB(255, 255, 255));

	std::cout << "IMAGES CREATED WITH SIZE: " << _areaSize.width << "x" << _areaSize.height << std::endl;

	_lastUpdatedRegion = cvRect(0, 0, rect.get_width(), rect.get_height());
	_isWindowInitialized = false;
}

TestArea::~TestArea() {}

void TestArea::showContents() {
	static double estimationXRight = 0;
	static double estimationYRight = 0;
	static double estimationXLeft = 0;
	static double estimationYLeft = 0;

	Glib::RefPtr<Gdk::Window> window = get_window();
	if (window) {
		if (Application::status == Application::STATUS_PAUSED) {
			std::cout << "PAUSED, DRAWING HERE" << std::endl;

			Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(window);
			IplImage *repositioningImage = MainGazeTracker::instance().repositioningImage;
			CvRect bounds = cvRect((_areaSize.width - repositioningImage->width) / 2, (_areaSize.height - repositioningImage->height) / 2, repositioningImage->width, repositioningImage->height);

			Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
				(guint8 *)repositioningImage->imageData,
				Gdk::COLORSPACE_RGB,
				false,
				repositioningImage->depth,
				repositioningImage->width,
				repositioningImage->height,
				repositioningImage->widthStep
			);

			window->draw_pixbuf(gc, pixbuf, 0, 0, bounds.x, bounds.y, bounds.width, bounds.height, Gdk::RGB_DITHER_NONE, 0, 0);

			_lastUpdatedRegion.x = bounds.x;
			_lastUpdatedRegion.y = bounds.y;
			_lastUpdatedRegion.width = bounds.width;
			_lastUpdatedRegion.height = bounds.height;
		} else if (Application::status == Application::STATUS_CALIBRATING){	// Calibration
			Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(window);
			Point calibrationPoint = MainGazeTracker::instance().calibrationPointer->getPosition();

			//if (Application::status == Application::STATUS_TESTING) {
			//	std::cout << "EXPECTED OUTPUT: ("<< calibrationPoint.x << ", " << calibrationPoint.y << ")" << std::endl << std::endl;
			//}

			// If the coordinates are beyond bounds, calibration is finished
			if (calibrationPoint.x > 3000 || calibrationPoint.y > 3000) {
				MainGazeTracker::instance().calibrationPointer = NULL;
				return;
			}

			if (calibrationPoint.x > 0 && calibrationPoint.y > 0) {
				CvRect currentBounds = cvRect(calibrationPoint.x - 25, calibrationPoint.y - 25, 50, 50);
				CvRect targetBounds = cvRect(0, 0, 50, 50);

				if (currentBounds.x < 0) {
					currentBounds.width += currentBounds.x;		// Remove the amount from the width
					targetBounds.x -= currentBounds.x;
					targetBounds.width += currentBounds.x;
					currentBounds.x = 0;
				}

				if (currentBounds.y < 0) {
					currentBounds.height += currentBounds.y;		// Remove the amount from the height
					targetBounds.y -= currentBounds.y;
					targetBounds.height += currentBounds.y;
					currentBounds.y = 0;
				}

				if (currentBounds.width + currentBounds.x > _areaSize.width) {
					currentBounds.width = _areaSize.width - currentBounds.x;
				}

				if (currentBounds.height + currentBounds.y > _areaSize.height) {
					currentBounds.height = _areaSize.height - currentBounds.y;
				}

				// Clear only previously updated region
				clearLastUpdatedRegion();

				// Draw only the region which is to be updated
				Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
					(guint8 *)_target->imageData,
					Gdk::COLORSPACE_RGB,
					false,
					_target->depth,
					_target->width,
					_target->height,
					_target->widthStep
				);

				window->draw_pixbuf(gc, pixbuf, targetBounds.x, targetBounds.y, currentBounds.x, currentBounds.y, targetBounds.width, targetBounds.height, Gdk::RGB_DITHER_NONE, 0, 0);

				_lastUpdatedRegion.x = currentBounds.x;
				_lastUpdatedRegion.y = currentBounds.y;
				_lastUpdatedRegion.width = targetBounds.width;
				_lastUpdatedRegion.height = targetBounds.height;
			}
		} else if (Application::status == Application::STATUS_CALIBRATED) {
			Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(window);

			double alpha = 0.6;
			estimationXRight = (1 - alpha) * _trackerOutput->gazePoint.x + alpha * estimationXRight;
			estimationYRight = (1 - alpha) * _trackerOutput->gazePoint.y + alpha * estimationYRight;
			estimationXLeft = (1 - alpha) * _trackerOutput->gazePointLeft.x + alpha * estimationXLeft;
			estimationYLeft = (1 - alpha) * _trackerOutput->gazePointLeft.x + alpha * estimationYLeft;

			int estimationX = (estimationXRight + estimationXLeft) / 2;
			int estimationY = (estimationYRight + estimationYLeft) / 2;
			CvRect bounds = cvRect(estimationX - 25, estimationY - 25, 50, 50);

			if (bounds.x < 0) {
				bounds.width += bounds.x;	// Remove the amount from the width
				bounds.x = 0;
			}

			if (bounds.y < 0) {
				bounds.height += bounds.y;	// Remove the amount from the height
				bounds.y = 0;
			}

			if (bounds.width + bounds.x > _areaSize.width) {
				bounds.width = _areaSize.width - bounds.x;
			}

			if (bounds.height + bounds.y > _areaSize.height) {
				bounds.height = _areaSize.height - bounds.y;
			}

			clearLastUpdatedRegion();

			Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
				(guint8 *)_target->imageData,
				Gdk::COLORSPACE_RGB,
				false,
				_target->depth,
				_target->width,
				_target->height,
				_target->widthStep
			);

			window->draw_pixbuf(gc, pixbuf, 0, 0, bounds.x, bounds.y, bounds.width, bounds.height, Gdk::RGB_DITHER_NONE, 0, 0);

			_lastUpdatedRegion.x = bounds.x;
			_lastUpdatedRegion.y = bounds.y;
			_lastUpdatedRegion.width = bounds.width;
			_lastUpdatedRegion.height = bounds.height;
		} else {
			clearLastUpdatedRegion();
		}
	}
}

void TestArea::clearLastUpdatedRegion() {
	if (_lastUpdatedRegion.width > 0) {
		Glib::RefPtr<Gdk::Window> window = get_window();
		Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(window);

		Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(
			(guint8 *)_clearingImage->imageData,
			Gdk::COLORSPACE_RGB,
			false,
			_clearingImage->depth,
			_clearingImage->width,
			_clearingImage->height,
			_clearingImage->widthStep
		);

		window->draw_pixbuf(gc, pixbuf, 0, 0, _lastUpdatedRegion.x, _lastUpdatedRegion.y, _lastUpdatedRegion.width, _lastUpdatedRegion.height, Gdk::RGB_DITHER_NORMAL , 0, 0);

		//std::cout << "CLEARING THE AREA: " << _lastUpdatedRegion.x << ", " << _lastUpdatedRegion.y << ", " << _lastUpdatedRegion.width << ", " << _lastUpdatedRegion.height << "." << std::endl;

		_lastUpdatedRegion.x = 0;
		_lastUpdatedRegion.y = 0;
		_lastUpdatedRegion.width = 0;
		_lastUpdatedRegion.height = 0;
	}
}

bool TestArea::onIdle() {
	showContents();
	//queue_draw();
	return true;
}

bool TestArea::on_expose_event(GdkEventExpose *event) {
	//showContents();
	return true;
}

bool TestArea::on_button_press_event(GdkEventButton *event) {
	return true;
}

bool TestArea::on_button_release_event(GdkEventButton *event) {
	return true;
}

