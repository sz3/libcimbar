#pragma once

#include "color-util/type.hpp"
#include <opencv2/opencv.hpp>
#include <string>

namespace cimbar
{
	cv::Mat load_img(std::string path, const std::string& image_dir="");

	colorutil::Lab convertColor(uchar r, uchar g, uchar b);
	std::tuple<uchar,uchar,uchar> getColor(unsigned index);
	colorutil::Lab getLabColor(unsigned index);
	cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned color=0, const std::string& image_dir="");
}
