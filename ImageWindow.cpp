#include <QApplication>
#include <QDesktopWidget>

#include "ImageWindow.h"
#include "Application.h"
#include "utils.h"

ImageWindow::ImageWindow(bool debugWindow) {
	cv::Rect* geometry = Utils::getMainMonitorGeometry();
	cv::Size windowSize(geometry->width, geometry->height);
	
	// If it's a debug window, get the debug monitor geometry
	// and read the image resolution from video input
	if(debugWindow) {
		geometry = Utils::getDebugMonitorGeometry();
		windowSize = Application::Components::videoInput->debugFrame.size();
	}
	
	// Center window in the monitor it belongs to
	int x = geometry->x + (int) ((geometry->width - windowSize.width)/2);
	int y = geometry->y;
	
	this->setFixedSize(windowSize.width, windowSize.height);
	this->move(x, y);
	//this->setWindowTitle("Debug Info");

	imageWidget = new ImageWidget(0, windowSize);
	this->setCentralWidget(imageWidget);
}

ImageWindow::~ImageWindow() {}

void ImageWindow::showImage(cv::Mat image) {
	imageWidget->showImage(image);
}

