/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "bitmatrix.h"

#include <opencv2/opencv.hpp>

TEST_CASE( "bitmatrixTest/test_mat_to_bitbuffer", "[unit]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	// grayscale and threshold, to imitate the decoder process
	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitbuffer bb(symbols.rows * symbols.cols / 8);
	bitmatrix::mat_to_bitbuffer(symbols, bb.get_writer());

	// 62,8
	int start = 8254;
	assertEquals( 0xFF, bb.read(start, 8) );
	assertEquals( 0xFE, bb.read(start+1024, 8) );
	assertEquals( 0xFC, bb.read(start+2048, 8) );
	assertEquals( 0xF8, bb.read(start+3072, 8) );
	assertEquals( 0xF0, bb.read(start+4096, 8) );

	bitmatrix bm(bb, 1024, 1024, 61, 7);
	assertEquals( 0xFF, bm.get(1, 1, 8) );
	assertEquals( 0xFE, bm.get(1, 2, 8) );
	assertEquals( 0xFC, bm.get(1, 3, 8) );
	assertEquals( 0xF8, bm.get(1, 4, 8) );
	assertEquals( 0xF0, bm.get(1, 5, 8) );
}
