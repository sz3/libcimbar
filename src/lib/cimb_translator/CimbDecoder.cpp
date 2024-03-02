/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbDecoder.h"

#include "Cell.h"
#include "Common.h"
#include "Config.h"
#include "image_hash/hamming_distance.h"
#include "serialize/format.h"

#include <algorithm>
#include <iostream>
#include <tuple>
using std::get;
using std::string;

const int FIX_THRESH_HIGH = 245;
const int FIX_THRESH_LOW = 0;
const float BEST_COLOR_FLOOR = 48.0f;

namespace {
	unsigned squared_difference(int a, int b)
	{
		return std::pow(a - b, 2);
	}

	uchar fix_single_color(float c, float adjustUp, float down)
	{
		c -= down;
		c *= adjustUp;
		if (c > (FIX_THRESH_HIGH-down))
			c = 255;
		if (c < FIX_THRESH_LOW)
			c = 0;
		return (uchar)c;
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

// protected
color_correction& CimbDecoder::internal_ccm() const
{
	static thread_local color_correction _ccm;
	return _ccm;
}

// public
const color_correction& CimbDecoder::get_ccm() const
{
	// testing/debugging purposes only!!!!
	return internal_ccm();
}

void CimbDecoder::update_color_correction(cv::Matx<float, 3, 3>&& ccm)
{
	internal_ccm().update(std::move(ccm));
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

unsigned CimbDecoder::get_best_symbol(image_hash::ahash_result<cimbar::Config::cell_size()>& results, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown) const
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
		// skip over this drift_idx if it matches cooldown
		// we could be more clever to check for corners, but for now this is fine
		// ~0U is "unset"
		if (drift_idx == cooldown and drift_idx != 4) // don't skip the center, obvs
			continue;
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

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown) const
{
	image_hash::ahash_result<cimbar::Config::cell_size()> results = image_hash::fuzzy_ahash<cimbar::Config::cell_size()>(
		cell, _ahashThreshold, image_hash::ahash_result<cimbar::Config::cell_size()>::FAST
	);
	return get_best_symbol(results, drift_offset, best_distance, cooldown);
}

unsigned CimbDecoder::decode_symbol(const bitmatrix& cell, unsigned& drift_offset, unsigned& best_distance, unsigned cooldown) const
{
	int checkRule = cooldown == 0xFE? image_hash::ahash_result<cimbar::Config::cell_size()>::ALL : image_hash::ahash_result<cimbar::Config::cell_size()>::FAST;
	image_hash::ahash_result<cimbar::Config::cell_size()> results = image_hash::fuzzy_ahash<cimbar::Config::cell_size()>(cell, checkRule);
	return get_best_symbol(results, drift_offset, best_distance, cooldown);
}

std::tuple<uchar,uchar,uchar> CimbDecoder::fix_color(std::tuple<float,float,float> c, float adjustUp, float down) const
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

std::tuple<uchar,uchar,uchar> CimbDecoder::get_color(int i, unsigned color_mode) const
{
	return cimbar::getColor(i, _numColors, color_mode);
}

unsigned CimbDecoder::get_best_color(float r, float g, float b, unsigned color_mode) const
{
	// transform color with ccm
	if (internal_ccm().active())
	{
		std::tuple<float, float, float> color = internal_ccm().transform(r, g, b);
		r = std::get<0>(color);
		g = std::get<1>(color);
		b = std::get<2>(color);
	}

	float max = std::max({r, g, b, 1.0f});
	float min = std::min({r, g, b, BEST_COLOR_FLOOR});
	if (min >= max)
		min = 0;
	float adjust = 255.0/(max - min);

	std::tuple<uchar,uchar,uchar> c = fix_color({r, g, b}, adjust, min);

	unsigned best_fit = 0;
	float best_distance = 1000000;
	for (unsigned i = 0; i < _numColors; ++i)
	{
		std::tuple<uchar,uchar,uchar> candidate = get_color(i, color_mode);
		unsigned distance = check_color_distance(c, candidate);
		if (distance < best_distance)
		{
			best_fit = i;
			best_distance = distance;
		}
	}
	return best_fit;
}

std::tuple<uchar,uchar,uchar> CimbDecoder::avg_color(const Cell& color_cell) const
{
	// TODO: check/enforce dimensions of color_cell?
	// limit dimensions to ignore outer row/col. We want to look at the middle 6x6, or 3x3...
	Cell center = color_cell;
	center.crop(1, 1, color_cell.cols()-2, color_cell.rows()-2);
	return center.mean_rgb();
}

unsigned CimbDecoder::decode_color(const Cell& color_cell, unsigned color_mode) const
{
	if (_numColors <= 1)
		return 0;
	auto [r, g, b] = avg_color(color_cell);
	return get_best_color(r, g, b, color_mode);
}

bool CimbDecoder::expects_binary_threshold() const
{
	return _ahashThreshold >= 0xFE;
}

unsigned CimbDecoder::symbol_bits() const
{
	return _symbolBits;
}
