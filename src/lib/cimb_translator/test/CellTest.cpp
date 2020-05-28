#include "unittest.h"

#include "Cell.h"
#include "Common.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "CellTest/testEquivalence", "[unit]" )
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

// need to validate that offsets (e.g. Cell(x, y)) are right
// some of my tests suggest it's backwards?

TEST_CASE( "CellTest/testOffsets", "[unit]" )
{
	string sample_path = TestCimbar::getSample("mycell2.png");
	cv::Mat cell = cv::imread(sample_path);


}
