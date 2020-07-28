#pragma once

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

class CimbEncoder
{
public:
	CimbEncoder(unsigned symbol_bits=4, unsigned color_bits=2, bool dark=true);

	cv::Mat load_tile(unsigned symbol_bits, unsigned index);
	bool load_tiles(unsigned symbol_bits);

	const cv::Mat& encode(unsigned bits) const;

protected:
	std::vector<cv::Mat> _tiles;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
};
