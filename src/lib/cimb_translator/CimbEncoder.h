/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

class CimbEncoder
{
public:
	CimbEncoder(unsigned symbol_bits, unsigned color_bits, bool dark=true, unsigned color_mode=1);

	cv::Mat load_tile(unsigned symbol_bits, unsigned index);
	bool load_tiles(unsigned symbol_bits);

	const cv::Mat& encode(unsigned bits) const;

protected:
	std::vector<cv::Mat> _tiles;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
	unsigned _colorMode;
};
