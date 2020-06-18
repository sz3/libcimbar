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

	std::vector<unsigned> expected = {
	    0, 10, 61, 30, 22, 6, 29, 8, 26, 19, 1, 30, 20, 8, 0, 30, 8, 27, 9, 8, 4, 14, 27, 57, 12
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

TEST_CASE( "CimbReaderTest/testSampleMessy", "[unit]" )
{
	string sample_path = TestCimbar::getSample("4c-f1-e.jpg");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	std::vector<unsigned> expected = {
	    0, 10, 61, 30, 22, 6, 29, 8, 26, 19, 1, 30, 20, 8, 0, 30, 8, 27, 9, 8, 4, 14, 27, 57, 12
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
