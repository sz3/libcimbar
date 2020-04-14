#pragma once

#include "Corners.h"

#include <opencv2/opencv.hpp>

#include <string>

class Deskewer
{
public:
	Deskewer();

	cv::Mat deskew(std::string img, const Corners& corners);
	cv::Mat deskew(cv::Mat img, const Corners& corners);
	bool save(const cv::Mat& img, std::string path);

protected:
	int _totalSize = 1024;
	int _anchorSize = 30;
};
