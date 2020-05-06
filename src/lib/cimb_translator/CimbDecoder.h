#pragma once

#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>

class CimbDecoder
{
public:
	CimbDecoder(unsigned symbol_bits, unsigned color_bits);

	unsigned decode(const cv::Mat& color_cell);
	unsigned decode(const cv::Mat& cell, const cv::Mat& color_cell, unsigned& drift_offset);

	unsigned get_best_symbol(const std::array<uint64_t,9>& hashes, unsigned& best_distance);
	unsigned decode_symbol(const cv::Mat& cell, unsigned& drift_offset);

	unsigned get_best_color(unsigned char r, unsigned char g, unsigned char b) const;
	unsigned decode_color(const cv::Mat& cell, const std::pair<int, int>& drift);

protected:
	uint64_t get_tile_hash(unsigned symbol);
	bool load_tiles();

	unsigned check_color_distance(std::tuple<uchar,uchar,uchar> c, unsigned char r, unsigned char g, unsigned char b) const;
	unsigned char fix_color(unsigned char c, float adjust) const;

protected:
	std::vector<uint64_t> _tileHashes;
	unsigned _symbolBits;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
};
