/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbDecoder.h"

#include "Common.h"
#include "Cell.h"
#include "image_hash/hamming_distance.h"
#include "serialize/format.h"

#include <algorithm>
#include <iostream>
#include <tuple>
using std::get;
using std::string;

namespace {
	inline constexpr unsigned char operator"" _uchar(unsigned long long arg) noexcept
	{
		return static_cast<unsigned char>(arg);
	}

	template <typename T>
	unsigned squared_difference(T a, T b)
	{
		return std::pow(a - b, 2);
	}

	uchar fix_single_color(uchar c, float adjustUp, uchar down)
	{
		c -= down;
		c = (uchar)(c * adjustUp);
		if (c > (245 - down))
			c = 255;
		return c;
	}

	std::tuple<int,int,int> relative_color(std::tuple<uchar,uchar,uchar> c)
	{
		int r = std::get<0>(c);
		int g = std::get<1>(c);
		int b = std::get<2>(c);
		return {r - g, g - b, b - r};
	}

	unsigned color_diff(std::tuple<uchar,uchar,uchar> a, std::tuple<uchar,uchar,uchar> b)
	{
		std::tuple<int,int,int> rel1 = relative_color(a);
		std::tuple<int,int,int> rel2 = relative_color(b);
		return (
		    squared_difference(std::get<0>(rel1), std::get<0>(rel2)) +
		    squared_difference(std::get<1>(rel1), std::get<1>(rel2)) +
		    squared_difference(std::get<2>(rel1), std::get<2>(rel2))
		);
	}
}

CimbDecoder::CimbDecoder(unsigned symbol_bits, unsigned color_bits, bool dark, uchar ahashThreshold)
    : _symbolBits(symbol_bits)
    , _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
    , _dark(dark)
    , _ahashThreshold(ahashThreshold)
{
	load_tiles();
}

uint64_t CimbDecoder::get_tile_hash(unsigned symbol) const
{
	cv::Mat tile = cimbar::getTile(_symbolBits, symbol, _dark, _numColors);
	return image_hash::average_hash(tile);
}

bool CimbDecoder::load_tiles()
{
	unsigned numTiles = _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tileHashes.push_back(get_tile_hash(i));
	return true;
}

unsigned CimbDecoder::get_best_symbol(image_hash::ahash_result& results, unsigned& drift_offset, unsigned& best_distance) const
{
	drift_offset = 0;
	unsigned best_fit = 0;
	best_distance = 1000;
	// ahash_result will give us either 5 or 9 candidate hashes -- depending on whether we want to ignore the corners or not.
	// Because we're greedy (see the `return`), we will iterate out from the center
	// 4 == center.
	// 5, 7, 3, 1 == sides.
	// 8, 0, 2, 6 == corners.
	for (auto&& [drift_idx, h] : results)
	{
		for (unsigned i = 0; i < _tileHashes.size(); ++i)
		{
			unsigned distance = image_hash::hamming_distance(h, _tileHashes[i]);
			if (distance < best_distance)
			{
				best_distance = distance;
				best_fit = i;
				drift_offset = drift_idx;
				if (best_distance == 0)
					return best_fit;
			}
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance) const
{
	image_hash::ahash_result results = image_hash::fuzzy_ahash(cell, _ahashThreshold, image_hash::ahash_result::FAST);
	return get_best_symbol(results, drift_offset, best_distance);
}

unsigned CimbDecoder::decode_symbol(const bitmatrix& cell, unsigned& drift_offset, unsigned& best_distance) const
{
	image_hash::ahash_result results = image_hash::fuzzy_ahash(cell, image_hash::ahash_result::FAST);
	return get_best_symbol(results, drift_offset, best_distance);
}

std::tuple<uchar,uchar,uchar> CimbDecoder::fix_color(std::tuple<uchar,uchar,uchar> c, float adjustUp, uchar down) const
{
	return {
		fix_single_color(std::get<0>(c), adjustUp, down),
		fix_single_color(std::get<1>(c), adjustUp, down),
		fix_single_color(std::get<2>(c), adjustUp, down)
	};
}

unsigned CimbDecoder::check_color_distance(std::tuple<uchar,uchar,uchar> a, std::tuple<uchar,uchar,uchar> b) const
{
	return color_diff(a, b);
}

unsigned CimbDecoder::get_best_color(uchar r, uchar g, uchar b) const
{
	unsigned char max = std::max({r, g, b, 1_uchar});
	unsigned char min = std::min({r, g, b, 48_uchar});
	float adjust = 255.0;
	if (min >= max)
		min = 0;
	adjust /= (max - min);

	std::tuple<uchar,uchar,uchar> c = fix_color({r, g, b}, adjust, min);

	unsigned best_fit = 0;
	double best_distance = 1000000;
	for (int i = 0; i < _numColors; ++i)
	{
		std::tuple<uchar,uchar,uchar> candidate = cimbar::getColor(i, _numColors);
		unsigned distance = check_color_distance(c, candidate);
		if (distance < best_distance)
		{
			best_fit = i;
			best_distance = distance;
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_color(const cv::Mat& color_cell, const std::pair<int, int>& drift) const
{
	if (_numColors <= 1)
		return 0;

	// limit dimensions to ignore outer row/col. We want to look at the middle 6x6
	cv::Rect crop(2+drift.first, 2+drift.second, color_cell.cols-4, color_cell.rows-4);
	cv::Mat center = color_cell(crop);
	uchar r,g,b;
	std::tie(r, g, b) = Cell(center).mean_rgb(Cell::SKIP);
	return get_best_color(r, g, b);
}

unsigned CimbDecoder::decode_color(const Cell& color_cell, const std::pair<int, int>& drift) const
{
	if (_numColors <= 1)
		return 0;

	// limit dimensions to ignore outer row/col. We want to look at the middle 6x6
	Cell center = color_cell;
	center.crop(2+drift.first, 2+drift.second, color_cell.cols()-4, color_cell.rows()-4);
	uchar r,g,b;
	std::tie(r, g, b) = center.mean_rgb(Cell::SKIP);
	return get_best_color(r, g, b);
}

unsigned CimbDecoder::decode(const cv::Mat& color_cell) const
{
	unsigned drift_offset;
	unsigned distance;
	return decode(color_cell, color_cell, drift_offset, distance);
}

bool CimbDecoder::expects_binary_threshold() const
{
	return _ahashThreshold >= 0xFE;
}
