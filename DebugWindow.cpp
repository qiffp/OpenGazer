#include <QObject>
#include <QShortcut>
#include <QString>

#include "DebugWindow.h"
#include "Application.h"

DebugWindow::DebugWindow() : 
	_window(true)	// Create this window in the debug screen
{
	screenImage.create(cv::Size(_window.size().width(), _window.size().height()), CV_8UC3);
	
	QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_C), &_window);
	QObject::connect(shortcut, SIGNAL(activated()), Application::Components::mainTracker, SLOT(startCalibration()));
	
	shortcut = new QShortcut(QKeySequence(Qt::Key_T), &_window);
	QObject::connect(shortcut, SIGNAL(activated()), Application::Components::mainTracker, SLOT(startTesting()));
	
	shortcut = new QShortcut(QKeySequence(Qt::Key_X), &_window);
	QObject::connect(shortcut, SIGNAL(activated()), Application::Components::mainTracker, SLOT(choosePoints()));
	
	_window.show();
	_window.raise();
}

DebugWindow::~DebugWindow() {}

// Main processing function
void DebugWindow::display() {
	_window.showImage(Application::Components::videoInput->debugFrame);
	_window.setWindowTitle("Opengazer (" + QString::number(Application::Components::videoInput->frameRate, 'g', 4) + " fps)");
}


