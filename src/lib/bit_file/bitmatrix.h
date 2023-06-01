/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer.h"
#include <opencv2/opencv.hpp>

// wraps/inherits bitbuffer
// imposes dimensionality

class bitmatrix
{
public:
	template <typename BITSTREAM>
	static void mat_to_bitbuffer(const cv::Mat& img, BITSTREAM&& writer)
	{
		const uchar* p = img.ptr<uchar>(0);
		unsigned size = img.cols * img.rows;
		while (size >= 8)
		{
			// we're turning 1 uint64_t into 8 uint8_ts
			uint64_t mval;
			memcpy(&mval, p, sizeof mval);
			mval = mval & 0x101010101010101ULL;
			const uint8_t* cv = reinterpret_cast<const uint8_t*>(&mval);
			// TODO: what about endianness???
			uint8_t val = (
				cv[0] << 7 | cv[1] << 6 | cv[2] << 5 | cv[3] << 4 | cv[4] << 3 | cv[5] << 2 |
				cv[6] << 1 | cv[7]
			);
			writer << val;
			p += 8;
			size -= 8;
		}

		// remainder
		if (size > 0)
		{
			uint8_t val = 0;
			while (size > 0) {
				val |= (*p > 0) << size;
				++p;
				--size;
			}
			writer << val;
		}
	}

public:
	bitmatrix(const bitbuffer& buff, unsigned width, unsigned height, unsigned xstart=0, unsigned ystart=0)
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
	const bitbuffer& _buff;
	unsigned _xstart;
	unsigned _ystart;
	unsigned _width;
	unsigned _height;
};
