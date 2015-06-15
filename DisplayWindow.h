#pragma once
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <opencv2/opencv.hpp>

#include "GazeTracker.h"
#include "ImageWindow.h"

class DisplayArea {
public:
	DisplayArea(TrackerOutput *output);
	virtual ~DisplayArea();
	void showContents();
	void calculateNewFrogPosition();
	void clearLastUpdatedRegion();

private:
	TrackerOutput *_output;
	cv::Mat _current;
	cv::Mat *_repositioningImage;
	cv::Mat _origImage;
	cv::Mat _background;
	cv::Mat _frog;
	cv::Mat _target;
	cv::Mat _frogMask;
	cv::Mat _gaussianMask;
	cv::Mat _clearingImage;
	cv::Rect _lastUpdatedRegion;
	int _frogX;
	int _frogY;
	int _frogCounter;
	int _gameAreaX;
	int _gameAreaY;
	int _gameAreaWidth;
	int _gameAreaHeight;
	long _startTime;
	long _futureTime;
	long _tempTime;
	cv::Scalar _backgroundColor;
	bool _isWindowInitialized;

	bool onIdle();
};

class DisplayWindow: public Gtk::Window {
public:
	DisplayWindow(TrackerOutput *output);
	virtual ~DisplayWindow();
	cv::Mat *getCurrent();
	void setRepositioningImage(cv::Mat *image);
	void changeWindowColor(double illuminationLevel);

private:
	DisplayArea _picture;
	int _grayLevel;

	//Member widgets:
	//Gtk::Button _calibrateButton, _loadButton, _saveButton, _clearButton, _chooseButton;
	Gtk::VBox _vbox;
	//Gtk::HBox _buttonBar;

};
