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
    , _colorThreshold(80)
    , _dark(true)
{
	if (_dark)
		_backgroundColor = (0, 0, 0);
	else
		_backgroundColor = (0xFF, 0xFF, 0xFF);

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
	if (c < _colorThreshold)
		return (int)(c * adjust / 2);
	return (int)(c * adjust);
}

unsigned CimbDecoder::check_color_distance(cv::Vec3b c, unsigned char r, unsigned char g, unsigned char b) const
{
	return std::pow(c[2] - r, 2) + std::pow(c[1] - g, 2) + std::pow(c[0] - b, 2);
}

int CimbDecoder::get_best_color(unsigned char r, unsigned char g, unsigned char b) const
{
	if (_dark and r < _colorThreshold and g < _colorThreshold and b < _colorThreshold)
		return -1;

	unsigned char max = std::max(r, g);
	max = std::max(max, b);

	float adjust = 255.0 / max;
	r = fix_color(r, adjust);
	g = fix_color(g, adjust);
	b = fix_color(b, adjust);

	int best_fit = -1;
	unsigned best_distance = check_color_distance(_backgroundColor, r, g, b);
	if (best_distance < 2500)
		return best_fit;

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

unsigned CimbDecoder::decode_color(const cv::Mat& cell)
{
	if (_numColors <= 1)
		return 0;

	std::vector<int> counts;
	counts.resize(_numColors, 0);

	uchar r, g, b;
	for (int i = 1; i < cell.rows-1; ++i) // ignore top and bottom rows
	{
		const unsigned char* pixel = cell.ptr<uchar>(i);
		// ignore first and last column
		++pixel; ++pixel; ++pixel;
		for (int j = 1; j < cell.cols - 1; ++j)
		{
			b = *pixel++;
			g = *pixel++;
			r = *pixel++;
			int bestFit = get_best_color(r, g, b);
			if (bestFit >= 0)
			{
				if (++counts[bestFit] > 5)
					return bestFit;
			}
		}
	}
	return *std::max_element(counts.begin(), counts.end());
}

unsigned CimbDecoder::decode(const cv::Mat& cell, unsigned& distance) // drift_offset ?
{
	unsigned bits = decode_symbol(cell, distance);
	bits |= decode_color(cell) << _symbolBits;
	return bits;
}

unsigned CimbDecoder::decode(const cv::Mat& cell)
{
	unsigned distance;
	return decode(cell, distance);
}

