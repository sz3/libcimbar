/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellDrift.h"
#include "image_hash/ahash_result.h"
#include "image_hash/average_hash.h"
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>

class CimbDecoder
{
public:
	CimbDecoder(unsigned symbol_bits, unsigned color_bits, bool dark=true, uchar ahashThreshold=0);

	unsigned get_best_symbol(image_hash::ahash_result& results, unsigned& drift_offset, unsigned& best_distance) const;
	unsigned decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance) const;
	unsigned decode_symbol(const bitmatrix& cell, unsigned& drift_offset, unsigned& best_distance) const;

	unsigned get_best_color(uchar r, uchar g, uchar b) const;
	unsigned decode_color(const Cell& cell) const;

	bool expects_binary_threshold() const;
	unsigned symbol_bits() const;

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
