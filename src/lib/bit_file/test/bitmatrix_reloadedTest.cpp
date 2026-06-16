/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "bitmatrix.h"
#include "bitmatrix_reloaded.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "bitmatrix_reloadedTest/testLoad", "[unit]" )
{
	cv::Mat symbols = TestCimbar::loadSample("b/tr_0.png");
	assertEquals( 1024, symbols.cols );
	assertEquals( 1024, symbols.rows );

	cv::cvtColor(symbols, symbols, cv::COLOR_RGB2GRAY);
	cv::adaptiveThreshold(symbols, symbols, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 7, 0);

	bitmatrix_reloaded bm;
	assertEquals( 0, bm.load(symbols) );
	assertEquals( 256, bm.sectors().size() ); //16x16

	// test each 64x64 sector matches a bitbuffer loaded from that 16x16 section of image
	for (int row = 0; row < 16; ++row)
		for (int col = 0; col < 16; ++col)
		{
			cv::Rect bounds(row*64, col*64, 64, 64);
			cv::Mat sector = symbols(bounds).clone();

			bitbuffer bb(sector.rows * sector.cols / 8);
			bitmatrix::mat_to_bitbuffer(sector, bb.get_writer());

			unsigned si = row*16 + col;
			const bitbuffer2d& sct = bm.sectors()[si];
			assertEquals( sct.buffer().size(),  bb.buffer().size() );
			for (int i = 0; i < bb.buffer().size(); ++i)
				assertMsg( sct.buffer()[i] == bb.buffer()[i], fmt::format("buff check {},{},{}", row, col, i) );
		}



}
