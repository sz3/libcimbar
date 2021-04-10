/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbEncoder.h"

#include "Common.h"
#include "serialize/format.h"
#include <cmath>
#include <iostream>
using cv::Vec3b;
using std::string;

CimbEncoder::CimbEncoder(unsigned symbol_bits, unsigned color_bits, bool dark)
    : _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
    , _dark(dark)
{
	load_tiles(symbol_bits); // TODO: probably should be cached?
}

cimbar::tile CimbEncoder::load_tile(unsigned symbol_bits, unsigned index)
{
	unsigned symbol = index % _numSymbols;
	unsigned color = index / _numSymbols;
	return cimbar::getTile(symbol_bits, symbol, _dark, _numColors, color);
}

bool CimbEncoder::load_tiles(unsigned symbol_bits)
{
	unsigned numTiles = _numColors * _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tiles.push_back(load_tile(symbol_bits, i));
	return true;
}

const cimbar::tile& CimbEncoder::encode(unsigned bits) const
{
	bits = bits % _tiles.size();
	return _tiles[bits];
}
