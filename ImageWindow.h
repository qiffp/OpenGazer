#pragma once
#include "ImageWidget.h"
#include <QMainWindow>

class ImageWindow : public QMainWindow
{
public:
	ImageWindow(bool debugWindow = false);
	~ImageWindow();
	void showImage(cv::Mat image);

private:
	ImageWidget* imageWidget;
};


