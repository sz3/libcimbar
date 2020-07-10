#include "unittest.h"

#include "CimbReader.h"

#include "cimb_translator/CimbDecoder.h"
#include "serialize/format.h"
#include "serialize/str_join.h"
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
	int count = 0;
	std::vector<string> res;
	for (int c = 0; c < 22; ++c)
	{
		unsigned bits;
		unsigned index = cr.read(bits);
		res.push_back(fmt::format("{}={}", index, bits));
		++count;
	}

	string expected = "0=0 12399=29 12300=4 100=27 12299=35 12301=44 101=8 200=55 12199=40 12201=1 "
	        "1=10 300=3 12099=8 12200=37 12101=37 2=61 400=2 11999=6 12100=13 12001=12 102=30 500=53";
	assertEquals( expected, turbo::str::join(res) );

	unsigned bits;
	while (!cr.done())
	{
		cr.read(bits);
		++count;
	}
	assertTrue(cr.done());
	assertEquals(12400, count);
}

TEST_CASE( "CimbReaderTest/testSampleMessy", "[unit]" )
{
	string sample_path = TestCimbar::getSample("4c-f1-e.jpg");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	int count = 0;
	std::vector<string> res;
	for (int c = 0; c < 22; ++c)
	{
		unsigned bits;
		unsigned index = cr.read(bits);
		res.push_back(fmt::format("{}={}", index, bits));
		++count;
	}

	string expected = "0=0 12399=29 12300=4 99=8 100=27 1=10 101=8 200=55 201=3 300=3 202=54 301=6 "
	        "400=2 401=1 402=20 501=53 500=53 102=30 2=61 98=50 97=8 198=22";
	assertEquals( expected, turbo::str::join(res) );

	unsigned bits;
	while (!cr.done())
		cr.read(bits);
	assertTrue(cr.done());
}
