#pragma once

#include <boost/scoped_ptr.hpp>

#include "utils.h"
#include "GaussianProcess.cpp"

/*
struct TrackerOutput {
	Point gazePoint;
	Point gazePointLeft;

	// Neural network
	Point nnGazePoint;
	Point nnGazePointLeft;
	Point target;
	Point actualTarget;
	int targetId;
	int frameId;
	//bool outputError;

	TrackerOutput(Point gazePoint, Point target, int targetId);
	void setActualTarget(Point actual);
	//void setErrorOutput(bool show);
	void setFrameId(int id);
};
*/

class GazeTrackerHistogramFeatures {
	typedef MeanAdjustedGaussianProcess<std::vector<std::vector<int> > > HistProcess;

public:
	GazeTrackerHistogramFeatures();
	bool isActive();
	void addExemplar();

	// Calibration error removal
	void removeCalibrationError(Point &estimate);
	void boundToScreenArea(Point &estimate);

	void draw();
	void process();
	void updateEstimations();
	
	Point getTarget(int id);
	int getTargetId(Point point);

private:


	std::vector<std::vector<int> > vectorOfVectors_horizontal, vectorOfVectors_vertical, 
                                                vectorOfVectors_horizontal_left, vectorOfVectors_vertical_left;

    std::vector<std::vector<std::vector<int> > > histPositionSegmentedPixels, histPositionSegmentedPixels_left;



    std::vector<std::vector<int> > vector_h_v_combined; // ESTO SE ENCONTRABA EN EL ANTERIOR Caltarget, Donde lo pongo ?



	std::vector<Point> _calibrationTargetPoints;

	std::vector<Point> _calibrationTargetPointsAllFrames;
	
	boost::scoped_ptr<HistProcess> _histX;
	boost::scoped_ptr<HistProcess> _histY;

	// ONUR DUPLICATED CODE FOR LEFT EYE
	boost::scoped_ptr<HistProcess> _histXLeft;
	boost::scoped_ptr<HistProcess> _histYLeft;

	// Calibration error removal
	double _betaX, _gammaX, _betaY, _gammaY, _sigv[100];	// Max 100 calibration points
	double _xv[100][2], _fvX[100], _fvY[100];

    static double histDistancePosition_x(std::vector<std::vector<int> > hist_horizontal, std::vector<std::vector<int> > hist_vertical);
    static double covariancefunction_hist_position_x(const std::vector<std::vector<int> > & hist_horizontal, const std::vector<std::vector<int> > & hist_vertical);

    static double histDistancePosition_y(std::vector<std::vector<int> > hist_horizontal, std::vector<std::vector<int> > hist_vertical);
    static double covariancefunction_hist_position_y(const std::vector<std::vector<int> > & hist_horizontal, const std::vector<std::vector<int> > & hist_vertical);


	void updateGaussianProcesses();
};
