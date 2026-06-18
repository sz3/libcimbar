/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer2d.h"
#include "serialize/format.h"
#include "util/compiler_constants.h"

#include <opencv2/opencv.hpp>

// this (ideally) will replace bitmatrix, once it works
// need bitbuffer2ds of size 64x64 == 512 bytes

struct bitmatrix_sector_info
{
	int sector;
	int xcoord;
	int ycoord;

	bool bad() const
	{
		return sector < 0;
	}
};

class bitmatrix_reloaded
{
public:
	class view
	{
	public:
		inline view(const bitmatrix_reloaded& bm, int x, int y)
			: _bm(bm)
			, _x(x)
			, _y(y)
		{}

		CIMBAR_ALWAYS_INLINE inline intx::uint128 read(unsigned cols, unsigned rows) const
		{
			return _bm.read2(_x, _y, cols, rows);
		}

	protected:

		const bitmatrix_reloaded& _bm;
		int _x;
		int _y;
	};

protected:
	static constexpr int SECTOR_FACTOR = 6;
	static constexpr int SECTOR_DIM = (1 << SECTOR_FACTOR); // 64

	inline void resize(unsigned width, unsigned height)
	{
		_width = width;
		_height = height;
		_numSectorsX = std::ceil(width*1.0 / SECTOR_DIM);
		_numSectorsY = std::ceil(height*1.0 / SECTOR_DIM);

		unsigned sectorCount = _numSectorsX * _numSectorsY;
		if (sectorCount != _sectors.size())
		{
			_sectors.resize(_numSectorsX*_numSectorsY);
			for (int i = 0; i < _sectors.size(); ++i)
				_sectors[i].buffer().resize(SECTOR_DIM*8, 0);
		}
	}

public:
	bitmatrix_reloaded(unsigned width=0, unsigned height=0)
	{
		resize(width, height);
	}

	bitmatrix_reloaded(const cv::Mat& img)
		: bitmatrix_reloaded(img.cols, img.rows)
	{
		load(img);
	}

	// return 0 or success, number of pixels unread on failure
	CIMBAR_FLATTEN inline unsigned load(const cv::Mat& img)
	{
		const uchar* p = img.ptr<uchar>(0);
		unsigned size = img.cols * img.rows;
		resize(img.cols, img.rows);

		size_t pos = 0;
		while (size >= 8)
		{
			// we're turning 1 uint64_t into 8 uint8_ts
			// then smashing it into 1 uint8_t
			uint64_t mval;
			memcpy(&mval, p, sizeof mval);
			mval = mval & 0x101010101010101ULL;
			const uint8_t* cv = reinterpret_cast<const uint8_t*>(&mval);
			// TODO: what about endianness???
			uint8_t val = (
				cv[0] << 7 | cv[1] << 6 | cv[2] << 5 | cv[3] << 4 | cv[4] << 3 | cv[5] << 2 |
				cv[6] << 1 | cv[7]
			);

			bitmatrix_sector_info si = get_sector(pos);
			if (si.bad() or si.sector >= _sectors.size())
				return size;
			unsigned internalPos = static_cast<unsigned>(si.xcoord+(si.ycoord*SECTOR_DIM));
			_sectors[si.sector].write_aligned_byte(val, internalPos);
			p += 8;
			size -= 8;
			pos += 8;
		}

		// remainder
		if (size > 0)
		{
			bitmatrix_sector_info si = get_sector(pos);
			if (si.bad() or si.sector >= _sectors.size())
				return size;

			uint8_t val = 0;
			while (size > 0)
			{
				val |= (*p > 0) << size;
				++p;
				--size;
			}
			unsigned internalPos = static_cast<unsigned>(si.xcoord+(si.ycoord*SECTOR_DIM));
			_sectors[si.sector].write_aligned_byte(val, internalPos);
		}
		return size;
	}

	CIMBAR_ALWAYS_INLINE inline bitmatrix_sector_info get_sector(int x, int y) const
	{
		if (x < 0 or y < 0)
			return bitmatrix_sector_info{-1,0,0};

		int sectorCol = x / SECTOR_DIM;
		int sectorRow = y / SECTOR_DIM;
		x = x % SECTOR_DIM;
		y = y % SECTOR_DIM;

		return bitmatrix_sector_info{
			.sector=static_cast<int>(sectorCol+(sectorRow*_numSectorsX)),
			.xcoord=x, .ycoord=y
		};
	}

	CIMBAR_ALWAYS_INLINE inline bitmatrix_sector_info get_sector(unsigned pos) const
	{
		int x = pos % _width;
		int y = pos / _width;
		return get_sector(x, y);
	}

	// instead of read_row(), we could maybe do a read_sector_mask(x, y, cols, rows)
	// with each sector returning a uint128?
	// then we or them all at the end? prob performs better...
	CIMBAR_ALWAYS_INLINE inline intx::uint128 read2(int x, int y, unsigned cols, unsigned rows) const
	{
		// read N bits from each sector...
		intx::uint128 total(0);

		bitmatrix_sector_info tl = get_sector(x,y);
		if (tl.sector >= (int)_sectors.size())
			return total;
		if (!tl.bad())
			total |= _sectors[tl.sector].read_sector_mask(tl.xcoord, tl.ycoord, cols, rows);

		bitmatrix_sector_info br = get_sector(x+cols-1, y+rows-1);
		if (br.bad())
			return total;

		if (tl.sector != br.sector)
		{
			if (br.sector < _sectors.size())
			{
				total |= _sectors[br.sector].read_sector_mask(br.xcoord - cols+1, br.ycoord - rows+1, cols, rows);
				//short circuit if sector index is 1 away...
				if (br.sector == tl.sector + 1 or br.sector == tl.sector + _numSectorsX)
					return total;
			}

			// finally, the slow path
			bitmatrix_sector_info tr = get_sector(x+cols-1, y);
			if (!tr.bad() and tr.sector < _sectors.size() and tr.sector != tl.sector and tr.sector != br.sector)
				total |= _sectors[tr.sector].read_sector_mask(tr.xcoord - cols+1, tr.ycoord, cols, rows);

			bitmatrix_sector_info bl = get_sector(x, y+rows-1);
			if (!bl.bad() and bl.sector < _sectors.size() and bl.sector != tl.sector and bl.sector != br.sector and bl.sector != tr.sector)
				total |= _sectors[bl.sector].read_sector_mask(bl.xcoord, bl.ycoord - rows+1, cols, rows);
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

	inline const std::vector<bitbuffer2d>& sectors() const
	{
		return _sectors;
	}

protected:
	std::vector<bitbuffer2d> _sectors;
	unsigned _width;
	unsigned _height;
	unsigned _numSectorsX;
	unsigned _numSectorsY;
};
