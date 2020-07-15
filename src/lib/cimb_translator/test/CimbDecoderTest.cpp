#include "unittest.h"

#include "CimbDecoder.h"

#include "cimb_translator/Common.h"
#include "serialize/format.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

TEST_CASE( "CimbDecoderTest/testSimpleDecode", "[unit]" )
{
	// a resize test would be good too, but funnily enough... it fails on symbol 6->5 right now...
	// it's as if the tile set is not optimal!
	CimbDecoder cd(4, 0);

	string root = TestCimbar::getProjectDir();
	for (int i = 0; i < 16; ++i)
	{
		cv::Mat tile = cimbar::getTile(4, i, true, 0, root);
		cv::Mat tenxten(10, 10, tile.type());
		tile.copyTo(tenxten(cv::Rect(cv::Point(0, 0), tile.size())));
		unsigned res = cd.decode(tenxten);
		assertEquals(i, res);
	}
}

TEST_CASE( "CimbDecoderTest/testPrethresholdDecode", "[unit]" )
{
	// in practice, it can be useful to decode preprocessed symbol tiles
	// the "0xFF" flag is passed to the average_hash function, which uses it to
	// (1) skip computing the mean cell (threshold) value
	// (2) compute the hash faster (exploiting that it's all 0s or all 1s)
	// But in this test, we just want to see it get the right answer.
	CimbDecoder cd(4, 0, true, 0xFF);

	string root = TestCimbar::getProjectDir();
	for (int i = 0; i < 16; ++i)
	{
		cv::Mat tile = cimbar::getTile(4, i, true, 0, root);
		cv::Mat tenxten(10, 10, tile.type());
		tile.copyTo(tenxten(cv::Rect(cv::Point(0, 0), tile.size())));

		// grayscale and threshold, since that's what average_hash needs
		cv::cvtColor(tenxten, tenxten, cv::COLOR_BGR2GRAY);
		cv::adaptiveThreshold(tenxten, tenxten, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 0);

		unsigned res = cd.decode(tenxten);
		assertEquals(i, res);
	}
}

TEST_CASE( "CimbDecoderTest/test_get_best_color__dark", "[unit]" )
{
	CimbDecoder cd(4, 2);

	// obvious ones
	assertEquals(2, cd.get_best_color(255, 0, 255));
	assertEquals(1, cd.get_best_color(255, 255, 0));
	assertEquals(0, cd.get_best_color(0, 255, 255));
	assertEquals(3, cd.get_best_color(0, 255, 0));

	// arbitrary edge cases. We can't really say anything about the value of these colors, but we can at least pick a consistent one
	assertEquals(3, cd.get_best_color(0, 0, 0));
	assertEquals(3, cd.get_best_color(70, 70, 70));

	// these we can use!
	assertEquals(3, cd.get_best_color(20, 200, 20));
	assertEquals(3, cd.get_best_color(50, 155, 50));

	assertEquals(2, cd.get_best_color(200, 30, 200));
	assertEquals(2, cd.get_best_color(155, 50, 155));

	assertEquals(1, cd.get_best_color(200, 155, 20));
	assertEquals(1, cd.get_best_color(155, 155, 50));

	assertEquals(0, cd.get_best_color(50, 155, 200));
	assertEquals(0, cd.get_best_color(50, 155, 155));
}

TEST_CASE( "CimbDecoderTest/testColorDecode", "[unit]" )
{
	CimbDecoder cd(4, 2);

	string root = TestCimbar::getProjectDir();
	cv::Mat tile = cimbar::getTile(4, 2, true, 2, root);
	cv::resize(tile, tile, cv::Size(10, 10));

	unsigned color = cd.decode_color(tile, {0, 0});
	assertEquals(2, color);
	unsigned res = cd.decode(tile);
	assertEquals(34, res);
}

TEST_CASE( "CimbDecoderTest/testAllColorDecodes", "[unit]" )
{
	CimbDecoder cd(4, 2);

	string root = TestCimbar::getProjectDir();
	for (int c = 0; c < 4; ++c)  // 2 color bits == 4 colors
		for (int i = 0; i < 16; ++i)
		{
			cv::Mat tile = cimbar::getTile(4, i, true, c, root);
			cv::Mat tenxten(10, 10, tile.type());
			tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));
			DYNAMIC_SECTION( "testColor " << c << ":" << i )
			{
				unsigned color = cd.decode_color(tenxten, {0, 0});
				assertEquals(c, color);
				unsigned res = cd.decode(tenxten);
				assertEquals(i+16*c, res);
			}
		}
}

TEST_CASE( "CimbDecoderTest/test_decode_symbol_sloppy", "[unit]" )
{
	CimbDecoder cd(4, 2);

	string sample_path = TestCimbar::getSample("mycell.png");
	cv::Mat cell = cv::imread(sample_path);
	cv::resize(cell, cell, cv::Size(10, 10));

	unsigned drift_offset;
	unsigned best_distance;
	unsigned res = cd.decode_symbol(cell, drift_offset, best_distance);
	assertEquals(4, res);
	assertEquals(2, drift_offset);
	assertEquals(16, best_distance);
}
