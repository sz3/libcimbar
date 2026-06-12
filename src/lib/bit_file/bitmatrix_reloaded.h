/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bitbuffer2d.h"
#include <opencv2/opencv.hpp>

// this will replace bitmatrix, once it works

// need bitbuffer2ds of size 64x64 == 512 bytes

class bitmatrix_reloaded
{
protected:
	constexpr int SECTOR_FACTOR = 6;
	constexpr int SECTOR_DIM = (1 << SECTOR_FACTOR); // 64

public:
	bitmatrix_reloaded(unsigned width, unsigned height)
		: _width(width)
		, _height(height)
		, _sectorsX(std::ceil(width*1.0 / SECTOR_DIM))
		, _sectorsY(std::ceil(height*1.0 / SECTOR_DIM))
	{
		std::cerr << fmt::format("bmr sectors: {},{}", _sectorsX, _sectorsY) << std::endl;
		_sectors.resize(_sectorsX*_sectorsY);
	}

	void load(const cv::Mat& img)
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

	inline unsigned read_once(unsigned x, unsigned y, unsigned num_bits, uint64_t& bits)
	{
		// 1 <= num_bits <= 8
		// always returns >=1 (forward progress)

		// do the sector lookup, then project x,y into it
		uint64_t bits = 0;

		unsigned sectorCol = x / _sectorsX;
		unsigned sectorRow = y / _sectorsY;
		x = x % _sectorsX;
		y = y % _sectorsY;

		unsigned i = sectorCol + (sectorRow*_sectorsX);
		if (i >= _sectors.size())
			return 0;

		bitbuffer2d& sector = _sectors[i];

		unsigned pos = x + (y * SECTOR_DIM);
		if (pos + num_bits > sector.size())
			num_bits = sector.size() - pos;
		bits = sector.read(pos, num_bits);
		return num_bits;
	}

	// prob move this into bitbuffer2d
	// negative x or y means we backfill -N rows/columns with 0s before placing bits
	// y+rows or x+cols > bounds means we still shift as if we have everything, but we
	//  leave 0s as padding.
	// this also dovetails with the "apron" approach, if we go there...
	inline intx::uint128 read_sector_mask(int x, int y, uint16_t cols, uint16_t rows)
	{
		intx::uint128 total(0);
		// get sector. Then read whatever we've got, but *only* within cols+rows
		for (uint16_t yr = 0; yr < rows; ++yr)
		{

		}

		return total;
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
			unsigned readSize = std::max(8, num_bits - count);
			unsigned bits = 0;
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
			intx::uint128 res = read_row(xcol, yrow, bits_x);
			total |= res << bitsRemaining;
		}
		return total;
	}

	// instead of read_row(), we could maybe do a read_sector_mask(x, y, cols, rows)
	// with each sector returning a uint128?
	// then we or them all at the end? prob performs better...
	intx::uint128 read2(unsigned x, unsigned y, uint16_t cols, uint16_t rows) const
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

protected:
	std::vector<bitbuffer2d> _sectors;
	unsigned _width;
	unsigned _height;
	unsigned _sectorsX;
	unsigned _sectorsY;
};
