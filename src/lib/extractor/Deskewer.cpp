/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Deskewer.h"

#include <iostream>

Deskewer::Deskewer(unsigned total_size, unsigned anchor_size)
    : _totalSize(total_size)
    , _anchorSize(anchor_size)
{
}

int Deskewer::total_size() const
{
	return _totalSize;
}

cv::Mat Deskewer::deskew(std::string img, const Corners& corners)
{
	cv::Mat mat = cv::imread(img);
	cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
	return deskew(mat, corners);
}

bool Deskewer::save(const cv::Mat& img, std::string path)
{
	cv::Mat bgr;
	cv::cvtColor(img, bgr, cv::COLOR_RGB2BGR);
	return cv::imwrite(path, img);
}
