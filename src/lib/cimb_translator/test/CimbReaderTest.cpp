#include "unittest.h"

#include "CimbReader.h"

#include "cimb_translator/CimbDecoder.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

TEST_CASE( "CimbReaderTest/testSample", "[unit]" )
{
	string sample_path = TestCimbar::getSample("4color-ecc40-fountain-0.png");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	unsigned bits1 = cr.read();
	assertEquals(0, bits1);

	unsigned bits2 = cr.read();
	assertEquals(10, bits2);

	unsigned bits3 = cr.read();
	assertEquals(61, bits3);

	unsigned bits4 = cr.read();
	assertEquals(30, bits4);

	while (!cr.done())
		cr.read();
	assertTrue(cr.done());
}

TEST_CASE( "CimbReaderTest/testSampleMessy", "[unit]" )
{
	string sample_path = TestCimbar::getSample("4color1e.png");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	std::vector<unsigned> expected = {
	    8, 50, 4, 47, 29, 23, 13, 50, 11, 54, 9, 41, 27, 34, 61, 48, 30, 23, 17, 40, 27, 54, 56, 51, 2
	};

	// read
	for (unsigned val : expected)
	{
		unsigned bits = cr.read();
		assertEquals(val, bits);
	}

	while (!cr.done())
		cr.read();
	assertTrue(cr.done());
}
