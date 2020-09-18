/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

namespace cimbar {

class window
{
public:
	virtual ~window() {}

	virtual bool is_good() const = 0;
	virtual bool should_close() const = 0;
	virtual void show(const cv::Mat& img, unsigned delay) = 0;
};

}
