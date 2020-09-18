/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "average_hash.h"

#include "cimb_translator/Common.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;

TEST_CASE( "averageHashTest/testLight", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, false);
	assertEquals(0x103070f1f3f7f, image_hash::average_hash(tile));

	tile = cimbar::getTile(4, 0, false);
	assertEquals(0x103070f1f3f7f, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/testDark", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	assertEquals(0xfffefcf8f0e0c080, image_hash::average_hash(tile));

	tile = cimbar::getTile(4, 0, true);
	assertEquals(0xfffefcf8f0e0c080, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/testResize", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);

	cv::Mat big;
	cv::resize(tile, big, cv::Size(32, 32));
	assertEquals(0xfffefcf8f0e0c080, image_hash::average_hash(big));
}
