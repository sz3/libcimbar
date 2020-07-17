#include "CimbDecoder.h"

#include "Common.h"
#include "Cell.h"
#include "CellDrift.h"
#include "image_hash/average_hash.h"
#include "image_hash/hamming_distance.h"
#include "serialize/format.h"

#include <algorithm>
#include <iostream>
#include <tuple>
using std::get;
using std::string;

namespace {
	unsigned squared_difference(uchar a, uchar b)
	{
		return std::pow(a - b, 2);
	}

	uchar fix_single_color(uchar c, float adjustUp, uchar down)
	{
		c -= down;
		return (uchar)(c * adjustUp);
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
	cv::Mat tile = cimbar::getTile(_symbolBits, symbol, _dark);
	return image_hash::average_hash(tile);
}

bool CimbDecoder::load_tiles()
{
	unsigned numTiles = _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tileHashes.push_back(get_tile_hash(i));
	return true;
}

unsigned CimbDecoder::get_best_symbol(const std::array<uint64_t,9>& hashes, unsigned& drift_offset, unsigned& best_distance) const
{
	drift_offset = 0;
	unsigned best_fit = 0;
	best_distance = 1000;
	// there are 9 candidate hashes. Because we're greedy (see the `return`), we should iterate out from the center
	// 4 == center.
	// 5, 7, 3, 1 == sides.
	// 8, 0, 2, 6 == corners.
	for (unsigned d : {4, 5, 7, 3, 1, 8, 0, 2, 6})
	{
		for (unsigned i = 0; i < _tileHashes.size(); ++i)
		{
			unsigned distance = image_hash::hamming_distance(hashes[d], _tileHashes[i]);
			if (distance < best_distance)
			{
				best_distance = distance;
				best_fit = i;
				drift_offset = d;
				if (best_distance == 0)
					return best_fit;
			}
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell, unsigned& drift_offset, unsigned& best_distance) const
{
	auto bits = image_hash::fuzzy_ahash(cell, _ahashThreshold);
	std::array<uint64_t,9> hashes = image_hash::extract_fuzzy_ahash(bits);
	/*for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = cell(crop);

		hashes.push_back(image_hash::average_hash(img));
	}*/
	return get_best_symbol(hashes, drift_offset, best_distance);
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
	return squared_difference(get<0>(a), get<0>(b)) + squared_difference(get<1>(a), get<1>(b)) + squared_difference(get<2>(a), get<2>(b));
}

unsigned CimbDecoder::get_best_color(uchar r, uchar g, uchar b) const
{
	unsigned char max = std::max({r, g, b});
	unsigned char min = std::min({r, g, b});
	float adjust = 255.0;
	if (max > min)
		adjust /= (max - min);

	std::tuple<uchar,uchar,uchar> c = fix_color({r, g, b}, adjust, min);

	unsigned best_fit = 0;
	double best_distance = 1000000;
	for (int i = 0; i < _numColors; ++i)
	{
		std::tuple<uchar,uchar,uchar> candidate = cimbar::getColor(i);
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
	std::tie(r, g, b) = Cell(center).mean_rgb();
	return get_best_color(r, g, b);
}

unsigned CimbDecoder::decode(const cv::Mat& cell, const cv::Mat& color_cell, unsigned& drift_offset, unsigned& best_distance) const
{
	unsigned bits = decode_symbol(cell, drift_offset, best_distance);
	bits |= decode_color(color_cell, CellDrift::driftPairs[drift_offset]) << _symbolBits;
	return bits;
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
