#include "unittest.h"

#include "average_hash.h"

#include "cimb_translator/CimbCommon.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;

TEST_CASE( "averageHashTest/testLight", "[unit]" )
{
	string dir = cimbar::getTileDir(4);
	cv::Mat tile = cimbar::getTile(dir, 0, false);
	assertEquals(0xfefcf8f0e0c08000, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/testDark", "[unit]" )
{
	string dir = cimbar::getTileDir(4);
	cv::Mat tile = cimbar::getTile(dir, 0, true);
	assertEquals(0x103070f1f3f7fff, image_hash::average_hash(tile));
}

TEST_CASE( "averageHashTest/testResize", "[unit]" )
{
	string dir = cimbar::getTileDir(4);
	cv::Mat tile = cimbar::getTile(dir, 0, true);

	cv::Mat big;
	cv::resize(tile, big, cv::Size(32, 32));
	assertEquals(0x103070f1f3f7fff, image_hash::average_hash(big));
}
