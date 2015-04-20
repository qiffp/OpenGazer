#pragma once

#include "ImageWindow.h"

class DebugWindow {
public:	
	cv::Mat screenImage;
	
	DebugWindow();
	~DebugWindow();
	
	// Display the debug info
	void display();
	
private:
	ImageWindow _window;
};


