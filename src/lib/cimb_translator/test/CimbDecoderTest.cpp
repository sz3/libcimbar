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
	CimbDecoder cd(4, 0);

	string root = TestCimbar::getProjectDir();
	for (int i = 0; i < 16; ++i)
	{
		cv::Mat tile = cimbar::getTile(4, i, true, 0, root);
		unsigned res = cd.decode(tile);
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
	assertEquals(0, cd.get_best_color(70, 70, 70));

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
	unsigned color = cd.decode_color(tile);
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
			DYNAMIC_SECTION( "testColor " << c << ":" << i )
			{
				unsigned color = cd.decode_color(tile);
				assertEquals(c, color);
				unsigned res = cd.decode(tile);
				assertEquals(i+16*c, res);
			}
		}
}

TEST_CASE( "CimbDecoderTest/test_decode_symbol_sloppy", "[unit]" )
{
	CimbDecoder cd(4, 2);

	string sample_path = TestCimbar::getSample("mycell.png");
	cv::Mat cell = cv::imread(sample_path);

	unsigned distance;
	unsigned res = cd.decode_symbol(cell, distance);
	assertEquals(17, distance);
	assertEquals(7, res);
}
