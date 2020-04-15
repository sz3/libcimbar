#include "CimbEncoder.h"

#include "CimbCommon.h"
#include "serialize/format.h"
#include <cmath>
#include <iostream>
using cv::Vec3b;
using std::string;

CimbEncoder::CimbEncoder(unsigned symbol_bits, unsigned color_bits)
    : _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
{
	load_tiles(CimbCommon::getTileDir(symbol_bits));
}

cv::Mat CimbEncoder::load_tile(string tile_dir, unsigned index)
{
	unsigned symbol = index % _numSymbols;
	unsigned color = index / _numSymbols;
	return CimbCommon::getTile(tile_dir, symbol, true, color);
}

// dir will need to be passed via env? Doesn't make sense to compile it in, and doesn't *really* make sense to use cwd
bool CimbEncoder::load_tiles(std::string tile_dir)
{
	unsigned numTiles = _numColors * _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tiles.push_back(load_tile(tile_dir, i));
	return true;
}

const cv::Mat& CimbEncoder::encode(unsigned bits) const
{
	unsigned symbol = bits % _numSymbols;
	unsigned color = bits / _numSymbols;
	return _tiles[symbol];
}
