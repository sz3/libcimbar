/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Deskewer.h"
#include "cimb_translator/Config.h"

#include <iostream>

Deskewer::Deskewer(unsigned image_width, unsigned image_height, unsigned anchor_size)
	: _imageWidth(image_width? image_width : cimbar::Config::image_size_x())
	, _imageHeight(image_height? image_height : cimbar::Config::image_size_y())
	, _anchorSize(anchor_size? anchor_size : cimbar::Config::anchor_size())
{
}

unsigned Deskewer::image_width() const
{
	return _imageWidth;
}

unsigned Deskewer::image_height() const
{
	return _imageHeight;
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
