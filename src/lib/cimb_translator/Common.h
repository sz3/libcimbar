/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "res/load.h"
#include <opencv2/opencv.hpp>
#include <string>

namespace cimbar {

template <>
struct load<cv::Mat>
{
	static cv::Mat load_img(std::string path);
	static cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned num_colors=4, unsigned color=0);
};

}
