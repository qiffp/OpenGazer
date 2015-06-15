#pragma once
#include "utils.h"

#define VECTOR_SIZE 1

class ExtractEyeFeaturesSegmentation {

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
    cv::Mat* eyeGraySegmented, * eyeGraySegmented_left;

    static const int eyeDX;
    static const int eyeDY;
    static const cv::Size eyeSize;

public:

    std::vector<int> vector_horizontal, vector_vertical, vector_horizontal_left, vector_vertical_left;

    std::vector<std::vector<int> > histPositionSegmentedPixels, histPositionSegmentedPixels_left;

    ExtractEyeFeaturesSegmentation();
    void process();
    void draw(); 
    void processToExtractFeatures();
    cv::Mat constructTemplateDisk(int);

    // FALTA POR HACER LAS FUNCIONES DE MAS A BAJO!!

    cv::Mat Segmentation(cv::Mat);
    cv::Mat SegmentationWithOutputVideo(cv::Mat, cv::Mat);
    int CalculateOptimalThreshold(cv::Mat);
    int Otsu(std::vector<int>, int);
    cv::Mat CreateTemplateGausian2D(cv::Mat);
    void Histogram(cv::Mat, cv::Mat, cv::Mat, std::vector<int> *, std::vector<int> *, std::vector<std::vector<int> >*);
    void SortHistogram(std::vector<int> *, std::vector<int> *, std::vector<std::vector<int> >*);

};