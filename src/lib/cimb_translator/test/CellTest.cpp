/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "Cell.h"
#include "Common.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "CellTest/testRgbMatchesOpenCV", "[unit]" )
{
	cv::Mat cell = TestCimbar::loadSample("mycell.png");
	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(cell).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[0], (unsigned)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (unsigned)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[2], (unsigned)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	cv::Rect crop(125, 8, 8, 8);
	cv::Mat cell = img(crop);
	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(cell).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[0], (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[2], (int)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets.Contiguous", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	cv::Rect crop(125, 8, 8, 8);
	cv::Mat cell = img(crop);
	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(img, 125, 8, 8, 8).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[0], (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[2], (int)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets.Asymmetric", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	cv::Rect crop(125, 8, 4, 6);
	cv::Mat cell = img(crop);

	auto [r, g, b] = Cell(cell).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertEquals( 191, (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertEquals( 191, (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertEquals( 0, (int)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets.Asymmetric.Contiguous", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	cv::Rect crop(125, 8, 4, 6);
	cv::Mat cell = img(crop);

	auto [r, g, b] = Cell(img, 125, 8, 4, 6).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertEquals( 191, (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertEquals( 191, (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertEquals( 0, (int)b );
	}
}
