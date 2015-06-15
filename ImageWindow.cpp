#include <QApplication>
#include <QDesktopWidget>

#include "ImageWindow.h"
#include "Application.h"
#include "utils.h"

ImageWindow::ImageWindow(int screenIndex, bool debugSize) {
	cv::Rect* geometry = Utils::getSecondaryMonitorGeometry();

	if(screenIndex == 1) {
		geometry = Utils::getDebugMonitorGeometry();
		std::cout << "Debug screen!" << std::endl;
	}
	
	std::cout << "Screen geometry: size=" << geometry->width << "x" << geometry->height << " position=" << geometry->x << "," << geometry->y<< std::endl;
	std::cout << "Screen index=" << screenIndex << std::endl;
	
	cv::Size windowSize(geometry->width, geometry->height);
	
	// If it's a debug window, get the debug monitor geometry
	// and read the image resolution from video input
	if(debugSize) {
		windowSize = Application::Components::videoInput->debugFrame.size();
	}
	
	// Center window in the monitor it belongs to
	int x = geometry->x + (int) ((geometry->width - windowSize.width)/2);
	int y = geometry->y;
	
	this->setFixedSize(windowSize.width, windowSize.height);
	this->move(x, y);
	
	imageWidget = new ImageWidget(0, windowSize);
	this->setCentralWidget(imageWidget);
}

ImageWindow::~ImageWindow() {}

void ImageWindow::showImage(cv::Mat image) {
	imageWidget->showImage(image);
}

