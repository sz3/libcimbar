/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CimbEncoder.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

namespace {
	cv::Mat to_mat(cv::Mat img)
	{
		return img;
	}

	cv::Mat to_mat(cimbar::texture img)
	{
		std::string filename = "/tmp/cimbar_test_out.png";
		cimbar::imwrite(filename, img);
		return cv::imread(filename);
	}
}


TEST_CASE( "CimbEncoderTest/testSimple", "[unit]" )
{
	InitWindow(1024, 1024, "test raylib");

	CimbEncoder cw(4, 0);
	cimbar::image res = cw.encode(14);

	assertEquals(8, res.cols);

	cv::Mat expected = to_mat(cimbar::getTile(4, 14, true));
	REQUIRE(cv::sum(expected != res) == cv::Scalar(0,0,0,0));
}

TEST_CASE( "CimbEncoderTest/testColor", "[unit]" )
{
	CimbEncoder cw(4, 3);
	cimbar::image res = cw.encode(55);

	assertEquals(8, res.cols);
	/*
	cv::Mat expected = cimbar::getTile(4, 7, true, 8, 3); // 3*16 + 7 == 55
	REQUIRE(cv::sum(expected != res) == cv::Scalar(0,0,0,0));
	*/
}

