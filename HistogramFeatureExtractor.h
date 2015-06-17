#pragma once
#include "utils.h"

#define VECTOR_SIZE 10

class HistogramFeatureExtractor {

	int sizeImageDisk;
	
    cv::Mat Matches[VECTOR_SIZE];
    cv::Mat MatchesSmoothed[VECTOR_SIZE];
    cv::Mat elipse_gray;
    cv::Mat irisTemplateDisk[VECTOR_SIZE];
    cv::Mat irisTemplate;
    cv::Mat Gaussian2D[VECTOR_SIZE];
    double MinVal[VECTOR_SIZE], MaxVal[VECTOR_SIZE];
    cv::Point MinLoc[VECTOR_SIZE], MaxLoc[VECTOR_SIZE];
    cv::Scalar color, a[VECTOR_SIZE];
	
    cv::Mat histogram_horizontal, histogram_vertical, histogram_horizontal_left, histogram_vertical_left;
	cv::Mat wholeSegmentation, wholeSegmentation_left;

public:
    std::vector<int> vector_horizontal, vector_vertical, vector_horizontal_left, vector_vertical_left;

    HistogramFeatureExtractor();
    ~HistogramFeatureExtractor(void);
    void process();
    void draw();
    void processToExtractFeatures();
    void constructTemplateDisk(int, int);
    void Segmentation(cv::Mat, cv::Mat);
    void CreateTemplateGausian2D(int);
    void Histogram(cv::Mat, cv::Mat, cv::Mat, std::vector<int>*, std::vector<int>*);

};