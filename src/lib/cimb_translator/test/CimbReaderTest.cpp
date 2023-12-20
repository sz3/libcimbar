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
	assertEquals(1, color_bits);

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

	string expected = "0=16 99=24 11680=19 11681=48 11900=44 11901=41 11904=28 11995=18 11996=24 "
			"11998=22 11999=6 12001=45 12004=22 12099=18 12195=9 12196=17 12200=21 12201=16 "
			"12298=48 12299=50 12300=46 12399=31";
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

	string expected = "0=16 1=44 99=24 100=44 600=49 601=54 711=46 712=9 11464=5 11576=48 11577=60 "
			"11687=57 11688=7 11689=48 11690=0 11798=31 11799=41 12297=62 12298=48 12299=50 "
			"12300=46 12399=31";
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

	std::array<unsigned, 6> expectedColors = {1, 1, 1, 2, 2, 0};
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

	std::array<unsigned, 6> expectedColors = {1, 1, 1, 2, 2, 0};
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

TEST_CASE( "CimbReaderTest/testCCM.VeryNecessary", "[unit]" )
{
	cv::Mat sample = TestCimbar::loadSample("6bit/4_30_f0_177_ccm.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	assertTrue( decoder._ccm.active() );

	std::stringstream ss;
	ss << decoder._ccm.mat();
	assertEquals("[1.0675567, 0.21678841, -0.013292357;\n"
				 " 0.023812667, 0.98703396, -0.0048082001;\n"
				 " 0, 0, 1.0017186]", ss.str());

	std::array<unsigned, 6> expectedColors = {1, 1, 1, 0, 0, 1}; // it's wrong, but it's consistent!
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
