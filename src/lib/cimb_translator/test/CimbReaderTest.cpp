/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "CimbReader.h"

#include "cimb_translator/CimbDecoder.h"
#include "serialize/format.h"
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

	class TestableCimbDecoder : public CimbDecoder
	{
	public:
		using CimbDecoder::CimbDecoder;
		using CimbDecoder::_ccm;
	};
}
#include "serialize/str_join.h"

TEST_CASE( "CimbReaderTest/testReadOnce", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	assertFalse(cr.done());

	// read. The top left of the sample image happens to be 0, so it's not very exciting...
	PositionData pos;
	unsigned bits = cr.read(pos);
	assertEquals(0, bits);
	assertEquals(0, pos.i);
	assertEquals(62, pos.x);
	assertEquals(8, pos.y);

	unsigned color_bits = cr.read_color(pos);
	assertEquals(0, color_bits);

	assertFalse(cr.done());
}

TEST_CASE( "CimbReaderTest/testSample", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	int count = 0;
	std::map<unsigned, unsigned> res;
	for (int c = 0; c < 22; ++c)
	{
		PositionData pos;
		unsigned bits = cr.read(pos);
		res[pos.i] = bits;

		unsigned color_bits = cr.read_color(pos);
		res[pos.i] |= color_bits << decoder.symbol_bits();
		++count;
	}

	string expected = "0=0 99=8 12097=9 12100=0 12196=1 12197=25 12198=33 12200=5 12201=0 12295=32 "
	        "12296=32 12297=46 12298=32 12299=34 12300=30 12301=32 12394=32 12395=57 "
	        "12396=37 12397=38 12398=10 12399=15";
	assertEquals( expected, turbo::str::join(res) );

	PositionData pos;
	while (!cr.done())
	{
		cr.read(pos);
		++count;
	}
	assertTrue(cr.done());
	assertEquals(12400, count);
}

TEST_CASE( "CimbReaderTest/testSampleMessy", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4_30_f0_627_extract.jpg");

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// read
	int count = 0;
	std::map<unsigned, unsigned> res;
	for (int c = 0; c < 22; ++c)
	{
		PositionData pos;
		unsigned bits = cr.read(pos);
		res[pos.i] = bits;

		unsigned color_bits = cr.read_color(pos);
		res[pos.i] |= color_bits << decoder.symbol_bits();
		++count;
	}

	string expected = "0=0 1=28 99=8 11900=28 12000=23 12001=29 12100=0 12101=0 12102=25 12200=5 "
	        "12201=0 12202=33 12297=46 12298=32 12299=34 12300=30 12301=32 12302=32 12396=37 12397=38 12398=10 12399=15";
	assertEquals( expected, turbo::str::join(res) );

	PositionData pos;
	while (!cr.done())
		cr.read(pos);
	assertTrue(cr.done());
}

TEST_CASE( "CimbReaderTest/testBad", "[unit]" )
{
	// this is a non-extracted image, and it's dimensions are too small.
	// should immediately bail
	cv::Mat sample = TestCimbar::loadSample("6bit/4_30_f2_246.jpg");

	CimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// refuse to do anything
	assertTrue( cr.done() );

	PositionData pos;
	assertEquals( 0, cr.read(pos) );

	assertTrue( cr.done() );
}

TEST_CASE( "CimbReaderTest/testCCM", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4_30_f0_627_extract.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	assertTrue( decoder._ccm.active() );

	std::stringstream ss;
	ss << decoder._ccm.mat();
	assertEquals("[1.5489368, 0.050406694, -0.016417533;\n"
	             " 0.0055368096, 1.5302141, -0.0011175937;\n"
	             " 0, 0, 1.4676259]", ss.str());

	std::array<unsigned, 6> expectedColors = {0, 0, 1, 0, 2, 0};
	for (unsigned i = 0; i < expectedColors.size(); ++i)
	{
		PositionData pos;
		cr.read(pos);
		unsigned color_bits = cr.read_color(pos);
		SECTION( fmt::format("color {}", i) ) {
			assertEquals(expectedColors[i], color_bits);
		}
	}
}

TEST_CASE( "CimbReaderTest/testCCM.Disabled", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4_30_f0_627_extract.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder, false, false);

	assertFalse( decoder._ccm.active() );

	std::array<unsigned, 6> expectedColors = {0, 0, 1, 0, 2, 0};
	for (unsigned i = 0; i < expectedColors.size(); ++i)
	{
		PositionData pos;
		cr.read(pos);
		unsigned color_bits = cr.read_color(pos);
		SECTION( fmt::format("color {}", i) ) {
			assertEquals(expectedColors[i], color_bits);
		}
	}
}
