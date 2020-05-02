#include "CimbDecoder.h"

#include "Common.h"
#include "CellDrift.h"
#include "image_hash/average_hash.h"
#include "image_hash/hamming_distance.h"
#include "serialize/format.h"

#include <algorithm>
#include <iostream>
using std::string;

namespace {
	cv::Vec3b mean_color(const cv::Mat& img)
	{
		int rows = img.rows;
		int cols = img.cols * img.channels();

		if (img.isContinuous())
		{
			cols *= rows;
			rows = 1;
		}  // a hack!

		unsigned blue = 0;
		unsigned green = 0;
		unsigned red = 0;
		unsigned count = 0;

		int i,j;
		for( i = 0; i < img.rows; ++i)
		{
			const uchar* p = img.ptr<uchar>(i);
			for (j = 0; j < cols; j+=3)
			{
				blue += p[j];
				green += p[j+1];
				red += p[j+2];
				count += 1;
			}
		}

		if (!count)
			return cv::Vec3b(0, 0, 0);

		return cv::Vec3b(blue/count, green/count, red/count);
	}
}

CimbDecoder::CimbDecoder(unsigned symbol_bits, unsigned color_bits)
    : _symbolBits(symbol_bits)
    , _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
    , _dark(true)
{
	load_tiles();
}

uint64_t CimbDecoder::get_tile_hash(unsigned symbol)
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

unsigned CimbDecoder::get_best_symbol(const std::array<uint64_t,9>& hashes, unsigned& drift_offset)
{
	drift_offset = 0;
	unsigned best_fit = 0;
	unsigned best_distance = 1000;
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
				if (best_distance < 8)
					return best_fit;
			}
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell, unsigned& drift_offset)
{
	auto bits = image_hash::fuzzy_ahash(cell);
	std::array<uint64_t,9> hashes = image_hash::extract_fuzzy_ahash(bits);
	/*for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = cell(crop);

		hashes.push_back(image_hash::average_hash(img));
	}*/
	return get_best_symbol(hashes, drift_offset);
}

unsigned char CimbDecoder::fix_color(unsigned char c, float adjust) const
{
	return (int)(c * adjust);
}

unsigned CimbDecoder::check_color_distance(cv::Vec3b c, unsigned char r, unsigned char g, unsigned char b) const
{
	return std::pow(c[2] - r, 2) + std::pow(c[1] - g, 2) + std::pow(c[0] - b, 2);
}

unsigned CimbDecoder::get_best_color(unsigned char r, unsigned char g, unsigned char b) const
{
	unsigned char max = std::max(r, g);
	max = std::max(max, b);
	max = std::max(max, uchar(1));

	float adjust = 255.0 / max;
	r = fix_color(r, adjust);
	g = fix_color(g, adjust);
	b = fix_color(b, adjust);

	int best_fit = 0;
	unsigned best_distance = 1000000;
	for (int i = 0; i < _numColors; ++i)
	{
		cv::Vec3b c = cimbar::getColor(i);
		unsigned distance = check_color_distance(c, r, g, b);
		if (distance < best_distance)
		{
			best_fit = i;
			best_distance = distance;
			if (best_distance < 2500)
				break;
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_color(const cv::Mat& color_cell, const std::pair<int, int>& drift)
{
	if (_numColors <= 1)
		return 0;

	// limit dimensions to ignore outer row/col. We want to look at the middle 6x6
	cv::Rect crop(2 + drift.first, 2 + drift.second, color_cell.cols - 4, color_cell.rows - 4);
	cv::Mat center = color_cell(crop);
	cv::Vec3b avgColor = mean_color(center);
	return get_best_color(avgColor[2], avgColor[1], avgColor[0]);
}

unsigned CimbDecoder::decode(const cv::Mat& cell, const cv::Mat& color_cell, unsigned& drift_offset)
{
	unsigned bits = decode_symbol(cell, drift_offset);
	bits |= decode_color(color_cell, CellDrift::driftPairs[drift_offset]) << _symbolBits;
	return bits;
}

unsigned CimbDecoder::decode(const cv::Mat& color_cell)
{
	unsigned distance;
	return decode(color_cell, color_cell, distance);
}

