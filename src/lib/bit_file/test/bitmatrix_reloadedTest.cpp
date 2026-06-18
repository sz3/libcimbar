/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "bitmatrix.h"
#include "bitmatrix_reloaded.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

	// the old "fuzzy_ahash" read logic
	intx::uint128 read_block(const bitmatrix& img, unsigned readlen)
	{
		intx::uint128 res(0);
		int bitpos = readlen*readlen;
		for (unsigned i = 0; i < readlen; ++i)
		{
			intx::uint128 r = img.get(0, i, readlen);
			bitpos -= readlen;
			res |= r << bitpos;
		}
		return res;
	}
}

TEST_CASE( "bitmatrix_reloadedTest/testLoad", "[unit]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitmatrix_reloaded bm;
	assertEquals( 0, bm.load(symbols) );
	assertEquals( 256, bm.sectors().size() ); //16x16

	// test each 64x64 sector matches a bitbuffer loaded from that 16x16 section of image
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 16; ++col)
		{
			cv::Rect bounds(col*64, row*64, 64, 64);
			cv::Mat sector = symbols(bounds).clone();

			//cv::imwrite(fmt::format("/tmp/sector{}-{}.png", row,col), sector);

			bitbuffer bb(sector.rows * sector.cols / 8);
			bitmatrix::mat_to_bitbuffer(sector, bb.get_writer());

			unsigned si = row*16 + col;
			const bitbuffer2d& sct = bm.sectors()[si];
			assertEquals( sct.buffer().size(), bb.buffer().size() );
			for (int i = 0; i < bb.buffer().size(); ++i)
				assertMsg( sct.buffer()[i] == bb.buffer()[i], fmt::format("buff check {},{},{}", row, col, i) );
		}

}


TEST_CASE( "bitmatrix_reloadedTest/testRead2", "[unit]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitmatrix_reloaded bm;
	assertEquals( 0, bm.load(symbols) );

	// single sector read
	{
		intx::uint128 res = bm.read2(26, 89, 8, 1);
		assertEquals( 0xFF, (uint64_t)res);
	}

	{
		intx::uint128 res = bm.read2(26, 90, 8, 1);
		assertEquals( 0xFE, (uint64_t)res);
	}

	{
		intx::uint128 res = bm.read2(26, 89, 8, 2);
		assertEquals( 0xFFFE, (uint64_t)res);
	}

	{
		intx::uint128 res = bm.read2(26, 89, 8, 8);
		assertEquals( 0xFFFEFCF8F0E0C080, (uint64_t)res);
	}

	// single sector read again
	{
		intx::uint128 res = bm.read2(80, 8, 8, 2);
		assertEquals( 0xFFFE, (uint64_t)res);
	}

	{
		intx::uint128 res = bm.read2(80, 8, 8, 8);
		assertEquals( 0xFFFEFCF8F0E0C080, (uint64_t)res);
	}

	// multi sectors
	{
		intx::uint128 res = bm.read2(62, 8, 8, 2);
		assertEquals( 0xFFFE, (uint64_t)res);
	}

	{
		intx::uint128 res = bm.read2(62, 8, 8, 8);
		assertEquals( 0xFFFEFCF8F0E0C080, (uint64_t)res);
	}
}

TEST_CASE( "bitmatrix_reloadedTest/testRead2.OOB", "[unit]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitmatrix_reloaded bm;
	assertEquals( 0, bm.load(symbols) );

	// oob top left
	{
		intx::uint128 res = bm.read2(-4, -4, 8, 8);
		assertEquals( 0x303, (uint64_t)res);
	}

	// oob left
	{
		intx::uint128 res = bm.read2(-4, 4, 8, 8);
		assertEquals( 0x303030303030303ULL, (uint64_t)res);
	}

	// oob top
	{
		intx::uint128 res = bm.read2(4, -4, 8, 8);
		assertEquals( 0xFFFF, (uint64_t)res);
	}

	// oob bottom right
	{
		intx::uint128 res = bm.read2(1020, 1020, 8, 8);
		assertEquals( 0xC0C0000000000000ULL, (uint64_t)res);
	}

	// oob bottom
	{
		intx::uint128 res = bm.read2(1010, 1020, 8, 8);
		assertEquals( 0xffff000000000000ULL, (uint64_t)res);
	}

	// oob right
	{
		intx::uint128 res = bm.read2(1020, 1010, 8, 8);
		assertEquals( 0xC0C0C0C0C0C0C0C0ULL, (uint64_t)res);
	}
}

TEST_CASE( "bitmatrix_reloadedTest/testRead2.Exhaustive", "[exhaustive]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitmatrix_reloaded bm;
	assertEquals( 0, bm.load(symbols) );

	bitbuffer2d bb(symbols.rows, symbols.cols);
	bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());

	// test every 8x8 and 10x10 read
	for (int i = 0; i < symbols.rows-10; ++i)
		for (int j = 0; j < symbols.cols-10; ++j)
		{
			{
				INFO("t8 at " << fmt::format("row {}, col {}", i, j));
				intx::uint128 expected = bb.read_sector_mask(j, i, 8, 8);
				intx::uint128 actual = bm.read2(j, i, 8, 8);
				assertEquals( expected[0], actual[0] );
				assertEquals( expected[1], actual[1] );
			}

			{
				INFO("t10 at " << fmt::format("row {}, col {}", i, j));
				intx::uint128 expected = bb.read_sector_mask(j, i, 10, 10);
				intx::uint128 actual = bm.read2(j, i, 10, 10);
				assertEquals( expected[0], actual[0] );
				assertEquals( expected[1], actual[1] );
			}

			{
				INFO("legacy at " << fmt::format("row {}, col {}", i, j));
				bitmatrix bbreader(bb, symbols.cols, symbols.rows, j, i);
				intx::uint128 expected = read_block(bbreader, 10);
				intx::uint128 actual = bm.read2(j, i, 10, 10);
				assertEquals( expected[0], actual[0] );
				assertEquals( expected[1], actual[1] );
			}
		}
}
