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

	unsigned decode(const cv::Mat& color_cell) const;
	template <typename MAT>
	unsigned decode(const MAT& cell, const cv::Mat& color_cell, unsigned& drift_offset, unsigned& best_distance) const;

	unsigned get_best_symbol(image_hash::ahash_result& results, unsigned& drift_offset, unsigned& best_distance) const;
	unsigned decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance) const;
	unsigned decode_symbol(const bitmatrix& cell, unsigned& drift_offset, unsigned& best_distance) const;

	unsigned get_best_color(uchar r, uchar g, uchar b) const;
	unsigned decode_color(const cv::Mat& cell, const std::pair<int, int>& drift) const;

	bool expects_binary_threshold() const;

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

template <typename MAT>
inline unsigned CimbDecoder::decode(const MAT& cell, const cv::Mat& color_cell, unsigned& drift_offset, unsigned& best_distance) const
{
	unsigned bits = decode_symbol(cell, drift_offset, best_distance);
	bits |= decode_color(color_cell, CellDrift::driftPairs[drift_offset]) << _symbolBits;
	return bits;
}
