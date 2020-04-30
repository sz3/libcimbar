#include "CimbDecoder.h"

#include "Common.h"
#include "image_hash/average_hash.h"
#include "image_hash/hamming_distance.h"

#include <algorithm>
#include <iostream>
using std::string;

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

unsigned CimbDecoder::get_best_symbol(uint64_t hash, unsigned& best_distance)
{
	best_distance = 1000;
	unsigned best_fit = 0;
	for (unsigned i = 0; i < _tileHashes.size(); ++i)
	{
		unsigned distance = image_hash::hamming_distance(hash, _tileHashes[i]);
		if (distance < best_distance)
		{
			best_distance = distance;
			best_fit = i;
			if (best_distance == 0)
				break;
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell, unsigned& distance)
{
	uint64_t hash = image_hash::average_hash(cell);
	return get_best_symbol(hash, distance);
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

unsigned CimbDecoder::decode_color(const cv::Mat& color_cell)
{
	if (_numColors <= 1)
		return 0;

	// limit dimensions to ignore outer row/col
	// when we have the drift, that will factor into this calculation as well
	cv::Rect crop(1, 1, color_cell.cols - 2, color_cell.rows - 2);
	cv::Mat center = color_cell(crop);
	cv::Scalar avgColor = cv::mean(center);
	return get_best_color(avgColor[2], avgColor[1], avgColor[0]);
}

unsigned CimbDecoder::decode(const cv::Mat& cell, const cv::Mat& color_cell, unsigned& distance) // drift_offset ?
{
	unsigned bits = decode_symbol(cell, distance);
	bits |= decode_color(color_cell) << _symbolBits;
	return bits;
}

unsigned CimbDecoder::decode(const cv::Mat& color_cell)
{
	unsigned distance;
	return decode(color_cell, color_cell, distance);
}

