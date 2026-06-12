/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer.h"
#include "intx/intx.hpp"

// wraps/inherits bitbuffer
// imposes dimensionality

class bitbuffer2d : public bitbuffer
{
public:
	bitbuffer2d(unsigned width, unsigned height)
		: bitbuffer(width*height)
		, _width(width)
		, _height(height)
	{
	}

	// negative x or y means we backfill -N rows/columns with 0s before placing bits
	// y+rows or x+cols > bounds means we still shift as if we have everything, but we
	//  leave 0s as padding.
	// this also dovetails with the "apron" approach, if we go there...
	inline intx::uint128 read_sector_mask(int x, int y, uint16_t cols, uint16_t rows)
	{
		cols = std::min<uint16_t>(cols, 10);
		rows = std::min<uint16_t>(rows, 10);

		intx::uint128 total(0);
		int endX = x+cols;
		int endY = y+rows;

		if (endX <= 0 or endY <= 0)
			return total;

		unsigned bitsLeft = cols*rows;
		if (y < 0)
			bitsLeft -= (-y * cols);

		int startX = std::max(0, x);
		int startY = std::max(0, y);
		uint16_t readSize = std::min<uint16_t>(cols, endX-startX);

		// get sector. Then read whatever we've got, but *only* within cols+rows
		for (int yr = startY; yr < endY; ++yr)
		{
			unsigned pos = startX + (yr * _width); // TODO: move this into the for
			intx::uint128 r = read<intx::uint128>(pos, readSize);
			bitsLeft -= cols;
			total |= r << bitsLeft;
		}

		return total;
	}

	unsigned width() const
	{
		return _width;
	}

	unsigned height() const
	{
		return _height;
	}

protected:
	unsigned _width;
	unsigned _height;
};
