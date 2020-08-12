#include "unittest.h"

#include "Cell.h"
#include "Common.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "CellTest/testRgbMatchesOpenCV", "[unit]" )
{
	string sample_path = TestCimbar::getSample("mycell2.png");
	cv::Mat cell = cv::imread(sample_path);

	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(cell).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[2], (unsigned)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (unsigned)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[0], (unsigned)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets", "[unit]" )
{
	string sample_path = TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png");
	cv::Mat img = cv::imread(sample_path);

	cv::Rect crop(125, 8, 8, 8);
	cv::Mat cell = img(crop);
	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(cell).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[2], (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[0], (int)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets.Contiguous", "[unit]" )
{
	string sample_path = TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png");
	cv::Mat img = cv::imread(sample_path);

	cv::Rect crop(125, 8, 8, 8);
	cv::Mat cell = img(crop);
	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(img, 125, 8, 8, 8).mean_rgb();

	DYNAMIC_SECTION( "r" )
	{
		assertAlmostEquals( expectedColor[2], (int)r );
	}
	DYNAMIC_SECTION( "g" )
	{
		assertAlmostEquals( expectedColor[1], (int)g );
	}
	DYNAMIC_SECTION( "b" )
	{
		assertAlmostEquals( expectedColor[0], (int)b );
	}
}

TEST_CASE( "CellTest/testRgbCellOffsets.Asymmetric", "[unit]" )
{
	string sample_path = TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png");
	cv::Mat img = cv::imread(sample_path);

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
	string sample_path = TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png");
	cv::Mat img = cv::imread(sample_path);

	cv::Rect crop(125, 8, 4, 6);
	cv::Mat cell = img(crop);
	cv::imwrite("/tmp/hello.png", cell);

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
