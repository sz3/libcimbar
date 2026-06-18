/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer.h"
#include "serialize/format.h"
#include "util/compiler_constants.h"

#include "intx/intx.hpp"

// wraps/inherits bitbuffer
// imposes dimensionality

class bitbuffer2d : public bitbuffer
{
public:
	bitbuffer2d(unsigned width=64, unsigned height=64)
		: bitbuffer(width*height / 8)
		, _width(width)
		, _height(height)
	{
	}

	// negative x or y means we backfill -N rows/columns with 0s before placing bits
	// y+rows or x+cols > bounds means we still shift as if we have everything, but we
	//  leave 0s as padding.
	// this also dovetails with the "apron" approach, if we go there...
	CIMBAR_ALWAYS_INLINE inline intx::uint128 read_sector_mask(int x, int y, unsigned cols, unsigned rows) const
	{
		cols = std::min(cols, 10U);
		rows = std::min(rows, 10U);

		intx::uint128 total(0);
		int endX = std::min<int>(x+cols, _width);
		int endY = std::min<int>(y+rows, _height);

		if (endX <= 0 or endY <= 0)
			return total;

		unsigned leftPad = 0;
		if (x < 0)
			leftPad = -x;
		unsigned bitsLeft = cols*rows;
		if (y < 0)
			bitsLeft -= (-y * cols);

		int startX = std::max(0, x);
		int startY = std::max(0, y);
		uint16_t readSize = std::min<uint16_t>(cols, endX-startX);

		//std::cerr << fmt::format("readSize {}, start:{},{}, end:{},{}", readSize, startX,startY, endX, endY) << std::endl;
		//std::cerr << fmt::format("bitsleft:{}", bitsLeft) << std::endl;

		// get sector. Then read whatever we've got, but *only* within cols+rows
		for (int yr = startY; yr < endY; ++yr, bitsLeft -= cols)
		{
			unsigned pos = startX + (yr * _width); // TODO: move this into the for
			intx::uint128 r = read<intx::uint128>(pos, readSize);
			total |= r << (bitsLeft - readSize - leftPad);
			//std::cerr << fmt::format("leftpad {}, bitsLeft {}, cols {}, readSize {}", leftPad, bitsLeft, cols, readSize) << std::endl;
			//std::cerr << "read of size " << readSize << " .." << (uint64_t)r << " ..." << bitsLeft << std::endl;
		}

		return total;
	}

	inline unsigned width() const
	{
		return _width;
	}

	inline unsigned height() const
	{
		return _height;
	}

protected:
	unsigned _width;
	unsigned _height;
};
