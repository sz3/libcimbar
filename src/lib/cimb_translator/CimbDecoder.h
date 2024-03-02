/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellDrift.h"
#include "Config.h"
#include "chromatic_adaptation/color_correction.h"
#include "image_hash/ahash_result.h"
#include "image_hash/average_hash.h"
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>

class CimbDecoder
{
public:
	CimbDecoder(unsigned symbol_bits, unsigned color_bits, bool dark=true, uchar ahashThreshold=0);

	const color_correction& get_ccm() const;
	void update_color_correction(cv::Matx<float, 3, 3>&& ccm);

	unsigned get_best_symbol(image_hash::ahash_result<cimbar::Config::cell_size()>& results, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown=0xFF) const;
	unsigned decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown=0xFF) const;
	unsigned decode_symbol(const bitmatrix& cell, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown=0xFF) const;

	std::tuple<uchar,uchar,uchar> get_color(int i, unsigned color_mode) const;
	std::tuple<uchar,uchar,uchar> avg_color(const Cell& color_cell) const;
	unsigned get_best_color(float r, float g, float b, unsigned color_mode) const;
	unsigned decode_color(const Cell& cell, unsigned color_mode) const;

	bool expects_binary_threshold() const;
	unsigned symbol_bits() const;

protected:
	color_correction& internal_ccm() const;

	uint64_t get_tile_hash(unsigned symbol) const;
	bool load_tiles();

	unsigned check_color_distance(std::tuple<uchar,uchar,uchar> a, std::tuple<uchar,uchar,uchar> b) const;
	std::tuple<uchar,uchar,uchar> fix_color(std::tuple<float,float,float> c, float adjustUp, float down) const;

protected:
	std::vector<uint64_t> _tileHashes;
	unsigned _symbolBits;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
	uchar _ahashThreshold;
};
