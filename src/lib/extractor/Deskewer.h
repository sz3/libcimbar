#pragma once

#include "Corners.h"

#include <opencv2/opencv.hpp>

#include <string>

class Deskewer
{
public:
	Deskewer(unsigned total_size=1024, unsigned anchor_size=30);

	cv::Mat deskew(std::string img, const Corners& corners);
	cv::Mat deskew(const cv::Mat& img, const Corners& corners);
	bool save(const cv::Mat& img, std::string path);

protected:
	int _totalSize;
	int _anchorSize;
};
