#pragma once

#include <opencv2/opencv.hpp>
#include <string>

namespace cimbar
{
	cv::Mat load_img(std::string path, const std::string& image_dir="");

	cv::Vec3b getColor(unsigned index);
	cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark=true, unsigned color=0, const std::string& image_dir="");
}
