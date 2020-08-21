/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CimbReader.h"

#include "cimb_translator/CimbDecoder.h"
#include "serialize/format.h"
#include "serialize/str_join.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <map>
#include <string>
using std::string;

namespace {
	std::ostream& operator<<(std::ostream& s, const std::pair<unsigned, unsigned>& p)
	{
		s << p.first << "=" << p.second;
		return s;
	}
}

TEST_CASE( "CimbReaderTest/testSample", "[unit]" )
{
	string sample_path = TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	int count = 0;
	std::map<unsigned, unsigned> res;
	for (int c = 0; c < 22; ++c)
	{
		unsigned bits;
		unsigned index = cr.read(bits);
		res[index] = bits;
		++count;
	}

	string expected = "0=0 99=8 12097=9 12100=0 12196=1 12197=25 12198=33 12200=5 12201=0 12295=32 "
	        "12296=32 12297=46 12298=32 12299=34 12300=30 12301=32 12394=32 12395=57 "
	        "12396=37 12397=38 12398=10 12399=15";
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
	string sample_path = TestCimbar::getSample("6bit/4_30_f0_627_extract.jpg");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	int count = 0;
	std::map<unsigned, unsigned> res;
	for (int c = 0; c < 22; ++c)
	{
		unsigned bits;
		unsigned index = cr.read(bits);
		res[index] = bits;
		++count;
	}

	string expected = "0=0 1=28 99=8 11900=28 12000=23 12001=29 12100=0 12101=0 12102=25 12200=5 "
	        "12201=0 12202=33 12297=46 12298=32 12299=34 12300=30 12301=32 12302=32 12396=37 12397=38 12398=10 12399=15";
	assertEquals( expected, turbo::str::join(res) );

	unsigned bits;
	while (!cr.done())
		cr.read(bits);
	assertTrue(cr.done());
}

TEST_CASE( "CimbReaderTest/testBad", "[unit]" )
{
	// this is a non-extracted image, and it's dimensions are too small.
	// should immediately bail
	string sample_path = TestCimbar::getSample("6bit/4_30_f2_246.jpg");
	cv::Mat sample = cv::imread(sample_path);

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// refuse to do anything
	assertTrue( cr.done() );

	unsigned bits;
	assertEquals( 0, cr.read(bits) );

	assertTrue( cr.done() );
}
