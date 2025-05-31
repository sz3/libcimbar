/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "SimpleDeskewer.h"

#include <opencv2/opencv.hpp>

class Deskewer : public SimpleDeskewer
{
public:
	using SimpleDeskewer::SimpleDeskewer;
	using SimpleDeskewer::deskew;

	cv::Mat deskew(std::string img, const Corners& corners);
	bool save(const cv::Mat& img, std::string path);

protected:
};

inline cv::Mat Deskewer::deskew(std::string img, const Corners& corners)
{
	cv::Mat mat = cv::imread(img);
	cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
	return deskew(mat, corners);
}

inline bool Deskewer::save(const cv::Mat& img, std::string path)
{
	cv::Mat bgr;
	cv::cvtColor(img, bgr, cv::COLOR_RGB2BGR);
	return cv::imwrite(path, img);
}
