/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "CimbDecoder.h"

#include "bit_file/bitbuffer.h"
#include "bit_file/bitmatrix.h"
#include "cimb_translator/Common.h"
#include "serialize/format.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

namespace {
	// for performance reasons, the high level Decoder/CimbReader does the decode in 2 parts. This is the one-shot version.
	unsigned decode(CimbDecoder& cd, const cv::Mat& tile10)
	{
		unsigned drift_offset;
		unsigned distance;
		unsigned bits = cd.decode_symbol(tile10, drift_offset, distance);

		auto [x,y] = CellDrift::driftPairs[drift_offset];
		cv::Rect crop(1+x, 1+y, tile10.cols-2, tile10.rows-2);
		cv::Mat tile8 = tile10(crop);

		bits |= cd.decode_color(tile8, 1) << cd.symbol_bits();
		return bits;
	}
}

TEST_CASE( "CimbDecoderTest/testSimpleDecode", "[unit]" )
{
	CimbDecoder cd(4, 0);

	for (unsigned i = 0; i < 16; ++i)
	{
		cv::Mat tile = cimbar::getTile(4, i, true);
		cv::Mat tenxten(10, 10, tile.type());
		tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));
		unsigned res = decode(cd, tenxten);
		assertEquals(i, res);
	}
}

TEST_CASE( "CimbDecoderTest/testPrethresholdDecode", "[unit]" )
{
	// validate the bitmatrix version acts as we expect
	CimbDecoder cd(4, 0, true, 0xFF);

	for (unsigned i = 0; i < 16; ++i)
	{
		cv::Mat tile = cimbar::getTile(4, i, true);
		cv::Mat tenxten(10, 10, tile.type(), cv::Scalar(0, 0, 0));
		tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));

		// grayscale and threshold, since that's what average_hash needs
		cv::cvtColor(tenxten, tenxten, cv::COLOR_RGB2GRAY);
		cv::adaptiveThreshold(tenxten, tenxten, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 0);

		bitbuffer bb((100/8) + 1);
		bitmatrix::mat_to_bitbuffer(tenxten, bb.get_writer());
		bitmatrix bm(bb, 10, 10);

		unsigned drift_offset;
		unsigned distance;
		unsigned res = cd.decode_symbol(bm, drift_offset, distance);
		assertEquals(i, res);
		assertEquals(4, drift_offset);
		assertEquals(0, distance);
	}
}

TEST_CASE( "CimbDecoderTest/test_get_best_color_mode0", "[unit]" )
{
	CimbDecoder cd(4, 2);

	// obvious ones
	assertEquals(2, cd.get_best_color(255, 0, 255, 0));
	assertEquals(1, cd.get_best_color(255, 255, 0, 0));
	assertEquals(0, cd.get_best_color(0, 255, 255, 0));
	assertEquals(3, cd.get_best_color(0, 255, 0, 0));

	// arbitrary edge cases. We can't really say anything about the value of these colors, but we can at least pick a consistent one
	assertEquals(0, cd.get_best_color(0, 0, 0, 0));
	assertEquals(0, cd.get_best_color(70, 70, 70, 0));

	// these we can use!
	assertEquals(3, cd.get_best_color(20, 200, 20, 0));
	assertEquals(3, cd.get_best_color(50, 155, 50, 0));

	assertEquals(2, cd.get_best_color(200, 30, 200, 0));
	assertEquals(2, cd.get_best_color(155, 50, 155, 0));

	assertEquals(1, cd.get_best_color(200, 155, 20, 0));
	assertEquals(1, cd.get_best_color(155, 155, 50, 0));

	assertEquals(0, cd.get_best_color(50, 155, 200, 0));
	assertEquals(0, cd.get_best_color(50, 155, 155, 0));
}

TEST_CASE( "CimbDecoderTest/test_get_best_color_mode1", "[unit]" )
{
	CimbDecoder cd(4, 2);

	// obvious ones
	assertEquals(3, cd.get_best_color(255, 0, 255, 1));
	assertEquals(2, cd.get_best_color(255, 255, 0, 1));
	assertEquals(1, cd.get_best_color(0, 255, 255, 1));
	assertEquals(0, cd.get_best_color(0, 255, 0, 1));

	// arbitrary edge cases. We can't really say anything about the value of these colors, but we can at least pick a consistent one
	assertEquals(0, cd.get_best_color(0, 0, 0, 1));
	assertEquals(0, cd.get_best_color(70, 70, 70, 1));

	// these we can use!
	assertEquals(0, cd.get_best_color(20, 200, 20, 1));
	assertEquals(0, cd.get_best_color(50, 155, 50, 1));

	assertEquals(3, cd.get_best_color(200, 30, 200, 1));
	assertEquals(3, cd.get_best_color(155, 50, 155, 1));

	assertEquals(2, cd.get_best_color(200, 155, 20, 1));
	assertEquals(2, cd.get_best_color(155, 155, 50, 1));

	assertEquals(1, cd.get_best_color(50, 155, 200, 1));
	assertEquals(1, cd.get_best_color(50, 155, 155, 1));
}

TEST_CASE( "CimbDecoderTest/testColorDecode", "[unit]" )
{
	CimbDecoder cd(4, 2);

	cv::Mat tile = cimbar::getTile(4, 2, true, 4, 2);
	cv::resize(tile, tile, cv::Size(10, 10));

	unsigned color = cd.decode_color(Cell(tile), 1);
	assertEquals(2, color);
	unsigned res = decode(cd, tile);
	assertEquals(34, res);
}

TEST_CASE( "CimbDecoderTest/testAllColorDecodes", "[unit]" )
{
	CimbDecoder cd(4, 2);

	for (unsigned c = 0; c < 4; ++c)  // 2 color bits == 4 colors
		for (unsigned i = 0; i < 16; ++i)
		{
			DYNAMIC_SECTION( "testColor " << c << ":" << i )
			{
				cv::Mat tile = cimbar::getTile(4, i, true, 4, c);
				cv::Mat tenxten(10, 10, tile.type(), {0,0,0});
				tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));

				unsigned color = cd.decode_color(Cell(tenxten), 1);
				assertEquals(c, color);
				unsigned res = decode(cd, tenxten);
				assertEquals(i+16*c, res);
			}
		}
}

TEST_CASE( "CimbDecoderTest/test_decode_symbol_sloppy", "[unit]" )
{
	CimbDecoder cd(4, 2);

	cv::Mat cell = TestCimbar::loadSample("mycell.png");

	unsigned drift_offset;
	unsigned best_distance;
	unsigned res = cd.decode_symbol(cell, drift_offset, best_distance);
	assertEquals(0, res);
	assertEquals(7, drift_offset);
	assertEquals(6, best_distance);
}
