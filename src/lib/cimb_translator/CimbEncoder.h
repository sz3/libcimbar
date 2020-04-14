#pragma once

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

class CimbEncoder
{
public:
	CimbEncoder(unsigned symbol_bits=4, unsigned color_bits=2);

	cv::Mat load_tile(std::string tile_dir, unsigned index);
	bool load_tiles(std::string tile_dir);

	const cv::Mat& encode(unsigned bits) const;

protected:
	std::vector<cv::Mat> _tiles;
	unsigned _numSymbols;
	unsigned _numColors;
};
