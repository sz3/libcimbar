/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer.h"

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

	unsigned get(unsigned x, unsigned y, unsigned bits) const
	{
		y += _ystart;
		x += _xstart;
		unsigned i = x + (y * _width);
		return _buff.read(i, bits);
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
