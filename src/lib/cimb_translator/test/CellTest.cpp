#include "unittest.h"

#include "Cell.h"
#include "Common.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
using std::string;


TEST_CASE( "CellTest/testRgbEquivalence", "[unit]" )
{
	string sample_path = TestCimbar::getSample("mycell2.png");
	cv::Mat cell = cv::imread(sample_path);

	cv::Scalar expectedColor = cv::mean(cell);

	auto [r, g, b] = Cell(cell).mean_rgb<100>();

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
