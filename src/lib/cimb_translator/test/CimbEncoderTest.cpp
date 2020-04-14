#include "unittest.h"

#include "CimbEncoder.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CimbEncoderTest/testSimple", "[unit]" )
{
	CimbEncoder cw(4, 0);
	cv::Mat res = cw.encode(14);

	// need a way to validate res's contents better -- imagehash again?
	// or use the CimbReader?
	// or ...??
	std::cout << res << std::endl;
	cv::imwrite("/tmp/test_cimb_tile.png", res);
}

TEST_CASE( "CimbEncoderTest/testColor", "[unit]" )
{
	CimbEncoder cw(4, 3);
	cv::Mat res = cw.encode(55);

	// need a way to validate res's contents better -- imagehash again?
	// or use the CimbReader?
	// or ...??
	std::cout << res << std::endl;
	cv::imwrite("/tmp/test_cimb_tile2.png", res);
}

