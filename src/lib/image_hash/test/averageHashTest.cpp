#include "unittest.h"

#include "average_hash.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;

namespace {
	string getTilePath(unsigned symbol_bits, unsigned index)
	{
		return fmt::format("{}/bitmap/{}/{:02x}.png", LIBCIMBAR_PROJECT_ROOT, symbol_bits, index);
	}
}

TEST_CASE( "averageHashTest/testSimple", "[unit]" )
{
	cv::Mat tile = cv::imread(getTilePath(4, 0));
	assertEquals(0xfefcf8f0e0c08000, image_hash::average_hash(tile));
}

