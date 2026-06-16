/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer2d.h"
#include "serialize/format.h"
#include <opencv2/opencv.hpp>

#include <iostream>

// this will replace bitmatrix, once it works

// need bitbuffer2ds of size 64x64 == 512 bytes

struct bitmatrix_sector_info
{
	int32_t sector;
	int16_t xcoord;
	int16_t ycoord;
	unsigned pos;

	bool bad() const
	{
		return sector < 0;
	}
};

class bitmatrix_reloaded
{
protected:
	static constexpr int SECTOR_FACTOR = 6;
	static constexpr int SECTOR_DIM = (1 << SECTOR_FACTOR); // 64

	void resize(unsigned width, unsigned height)
	{
		std::cerr << fmt::format("bmr sectors: {},{}", width, height) << std::endl;
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

	// return 0 or success, number of pixels unread on failure
	unsigned load(const cv::Mat& img)
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
			_sectors[si.sector].write_aligned_byte(val, si.pos);
			p += 8;
			size -= 8;
			pos += 8;
		}

		// remainder
		if (size > 0)
		{
			bitmatrix_sector_info si = get_sector(pos);
			if (!si.bad() or si.sector >= _sectors.size())
				return false;

			uint8_t val = 0;
			while (size > 0)
			{
				val |= (*p > 0) << size;
				++p;
				--size;
			}
			_sectors[si.sector].write_aligned_byte(val, si.pos);
		}
		return size;
	}

	inline bitmatrix_sector_info get_sector(int x, int y) const
	{
		if (x < 0 or y < 0)
			return bitmatrix_sector_info{-1,0,0,0};

		unsigned sectorCol = x / SECTOR_DIM;
		unsigned sectorRow = y / SECTOR_DIM;
		x = x % SECTOR_DIM;
		y = y % SECTOR_DIM;

		return bitmatrix_sector_info{.sector=sectorRow+(sectorCol*_numSectorsY), .xcoord=x, .ycoord=y, .pos=x+(y*SECTOR_DIM)};
	}

	inline bitmatrix_sector_info get_sector(unsigned pos) const
	{
		int x = pos % _width;
		int y = pos / _width;
		return get_sector(x, y);
	}

	inline unsigned read_once(unsigned x, unsigned y, unsigned num_bits, uint64_t& bits) const
	{
		// 1 <= num_bits <= 8
		// always returns >=1 (forward progress)

		// do the sector lookup, then project x,y into it
		bits = 0;

		bitmatrix_sector_info si = get_sector(x, y);
		if (si.bad() or si.sector >= _sectors.size())
			return 0;

		const bitbuffer2d& sector = _sectors[si.sector];

		unsigned pos = si.pos;
		if (pos + num_bits > sector.size())
			num_bits = sector.size() - pos;
		bits = sector.read(pos, num_bits);
		return num_bits;
	}

	inline uint64_t read_row(unsigned x, unsigned y, unsigned num_bits) const
	{
		// 1 <= num_bits <= 64
		// read N bits from each sector...
		uint64_t total = 0;

		unsigned count = 0;
		while (count < num_bits)
		{
			// read at most 8 bits
			unsigned readSize = std::max<unsigned>(8, num_bits - count);
			uint64_t bits = 0;
			unsigned consumed = read_once(x+count, y, readSize, bits);
			count += consumed;

			// count == 1, num_bits == 8 -> shift 8 - 1 == 7
			total |= bits << (num_bits - count);
		}
		return total;
	}

	intx::uint128 read(unsigned x, unsigned y, uint16_t cols, uint16_t rows) const
	{
		// read N bits from each sector...
		intx::uint128 total(0);
		int bitsRemaining = cols*rows - 1;

		unsigned yrow = 0;
		unsigned xcol = 0;
		for (unsigned i = 0; i < rows and bitsRemaining >=0; ++i, bitsRemaining -= cols)
		{
			intx::uint128 res = read_row(xcol, yrow, cols);
			total |= res << bitsRemaining;
		}
		return total;
	}

	// instead of read_row(), we could maybe do a read_sector_mask(x, y, cols, rows)
	// with each sector returning a uint128?
	// then we or them all at the end? prob performs better...
	intx::uint128 read2(int x, int y, uint16_t cols, uint16_t rows) const
	{
		// read N bits from each sector...
		intx::uint128 total(0);

		// for 1-4 sectors
		// sectorRes = read_sector_mask()
		//total |= sectorRes;
		// ... and then opt to only compute when they're different sectors?

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

	const std::vector<bitbuffer2d>& sectors() const
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
