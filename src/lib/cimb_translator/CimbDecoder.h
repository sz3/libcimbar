#pragma once

#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>

class CimbDecoder
{
public:
	CimbDecoder(unsigned symbol_bits, unsigned color_bits, bool dark=true, uchar ahashThreshold=0);

	unsigned decode(const cv::Mat& color_cell) const;
	unsigned decode(const cv::Mat& cell, const cv::Mat& color_cell, unsigned& drift_offset, unsigned& best_distance) const;

	unsigned get_best_symbol(const std::array<uint64_t,9>& hashes, unsigned& drift_offset, unsigned& best_distance) const;
	unsigned decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance) const;

	unsigned get_best_color(uchar r, uchar g, uchar b) const;
	unsigned decode_color(const cv::Mat& cell, const std::pair<int, int>& drift) const;

protected:
	uint64_t get_tile_hash(unsigned symbol) const;
	bool load_tiles();

	unsigned check_color_distance(std::tuple<uchar,uchar,uchar> a, std::tuple<uchar,uchar,uchar> b) const;
	std::tuple<uchar,uchar,uchar> fix_color(std::tuple<uchar,uchar,uchar> c, float adjustUp, uchar down) const;

protected:
	std::vector<uint64_t> _tileHashes;
	unsigned _symbolBits;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
	uchar _ahashThreshold;
};
