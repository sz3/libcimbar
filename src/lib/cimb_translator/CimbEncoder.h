/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "image.h"

#include <string>
#include <vector>

class CimbEncoder
{
public:
	CimbEncoder(unsigned symbol_bits=4, unsigned color_bits=2, bool dark=true);

	cimbar::image load_tile(unsigned symbol_bits, unsigned index);
	bool load_tiles(unsigned symbol_bits);

	const cimbar::image& encode(unsigned bits) const;

protected:
	std::vector<cimbar::image> _tiles;
	unsigned _numSymbols;
	unsigned _numColors;
	bool _dark;
};
