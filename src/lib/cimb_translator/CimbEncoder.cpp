/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CimbEncoder.h"

// ifdef here?
#include "Common.h"
#include "serialize/format.h"
#include <cmath>
#include <iostream>
using cv::Vec3b;
using std::string;

namespace {
	std::vector<cimbar::tile>& tiles()
	{
		static std::vector<cimbar::tile> ts;
		return ts;
	}
}

CimbEncoder::CimbEncoder(unsigned symbol_bits, unsigned color_bits, bool dark)
    : _tiles(tiles())
    , _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
    , _dark(dark)
{
	load_tiles(symbol_bits); // TODO: smarter caching?
}

cimbar::tile CimbEncoder::load_tile(unsigned symbol_bits, unsigned index)
{
	unsigned symbol = index % _numSymbols;
	unsigned color = index / _numSymbols;
	// or ifdef here?
	return cimbar::getTile(symbol_bits, symbol, _dark, _numColors, color);
}

bool CimbEncoder::load_tiles(unsigned symbol_bits)
{
	unsigned numTiles = _numColors * _numSymbols;
	if (numTiles == _tiles.size())
		return false;

	_tiles.clear();
	for (unsigned i = 0; i < numTiles; ++i)
		_tiles.push_back(load_tile(symbol_bits, i));
	return true;
}

const cimbar::tile& CimbEncoder::encode(unsigned bits) const
{
	bits = bits % _tiles.size();
	return _tiles[bits];
}
