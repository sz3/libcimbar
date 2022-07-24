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

TEST_CASE( "averageHashTest/test5x5.Light", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(2, 0, false);
	assertEquals(0x8cef, image_hash::average_hash(tile));

	tile = cimbar::getTile(2, 1, false);
	assertEquals(0xf38c20, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/test5x5.Dark", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(2, 0, true);
	assertEquals(0x1ff7310, image_hash::average_hash(tile));

	tile = cimbar::getTile(2, 1, true);
	assertEquals(0x10c73df, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/test8x8.Light", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, false);
	assertEquals(0x103070f1f3f7f, image_hash::average_hash(tile));

	tile = cimbar::getTile(4, 1, false);
	assertEquals(0x7f3f1f0f07030100, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/test8x8.Dark", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	assertEquals(0xfffefcf8f0e0c080, image_hash::average_hash(tile));

	tile = cimbar::getTile(4, 1, true);
	assertEquals(0x80c0e0f0f8fcfeff, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/test8x8.Resize", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);

	cv::Mat big;
	cv::resize(tile, big, cv::Size(32, 32));
	assertEquals(0xfffefcf8f0e0c080, image_hash::average_hash(big));
}

