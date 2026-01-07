/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Deskewer.h"

#include <opencv2/opencv.hpp>

class DeskewerPlus : public Deskewer
{
public:
	using Deskewer::Deskewer;
	using Deskewer::deskew;

	cv::Mat deskew(std::string img, const Corners& corners, unsigned padding=0);
	bool save(const cv::Mat& img, std::string path);

protected:
};

inline cv::Mat DeskewerPlus::deskew(std::string img, const Corners& corners, unsigned padding)
{
	cv::Mat mat = cv::imread(img);
	cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
	return Deskewer::deskew(mat, corners, padding);
}

inline bool DeskewerPlus::save(const cv::Mat& img, std::string path)
{
	cv::Mat bgr;
	cv::cvtColor(img, bgr, cv::COLOR_RGB2BGR);
	return cv::imwrite(path, img);
}
