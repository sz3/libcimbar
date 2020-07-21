#pragma once

#include "bitbuffer.h"

// wraps/inherits bitbuffer
// imposes dimensionality

template <unsigned _size_hint=1550>
class bitmatrix
{
public:
	bitmatrix(const bitbuffer<_size_hint>& buff, unsigned width, unsigned height,  unsigned xstart=0, unsigned ystart=0)
		: _buff(buff)
		, _xstart(xstart)
		, _ystart(ystart)
		, _width(width)
		, _height(height)
	{
	}

	unsigned get(unsigned x, unsigned y, unsigned bits) const
	{
		y += _ystart;
		x += _xstart;
		unsigned i = y + (x * _height);
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
	bitbuffer<_size_hint> _buff;
	unsigned _xstart;
	unsigned _ystart;
	unsigned _width;
	unsigned _height;
};
