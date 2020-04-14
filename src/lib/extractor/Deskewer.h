#pragma once

#include "Corners.h"

#include <opencv2/opencv.hpp>

#include <string>

class Deskewer
{
public:
	Deskewer();

	bool deskew(std::string img, const Corners& corners);
	bool deskew(cv::Mat img, const Corners& corners);

protected:
	int _totalSize = 1024;
	int _anchorSize = 30;
};
