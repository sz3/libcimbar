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
	cv::Mat sample = TestCimbar::loadSample("b/ex2434.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// this is the header value for the sample -- we could imitate what the Decoder does
	// and compute it from the symbols, but that seems like overkill for this test.
	FountainMetadata md(0, 23586, 7);
	cr.update_metadata((char*)md.data(), md.md_size);
	cr.init_ccm(2, cimbar::Config::interleave_blocks(), cimbar::Config::interleave_partitions(), cimbar::Config::fountain_chunks_per_frame(6, false));

	assertTrue( decoder._ccm.active() );

	std::stringstream ss;
	ss << decoder._ccm.mat();
	assertEquals("[2.3991191, -0.41846275, -0.54654282;\n "
				 "-0.42976046, 2.632102, -0.76466882;\n "
				 "-0.54299992, -0.20199311, 2.2753253]", ss.str());

	std::array<unsigned, 6> expectedColors = {0, 1, 1, 2, 2, 2};
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
	cv::Mat sample = TestCimbar::loadSample("b/ex2434.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder, false, false);

	assertFalse( decoder._ccm.active() );

	std::array<unsigned, 6> expectedColors = {0, 1, 1, 2, 2, 2};
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
	cv::Mat sample = TestCimbar::loadSample("b/ex380.jpg");

	TestableCimbDecoder decoder(4, 2);
	CimbReader cr(sample, decoder);

	// this is the header value for the sample -- we could imitate what the Decoder does
	// and compute it from the symbols, but that seems like overkill for this test.
	FountainMetadata md(0, 23586, 7);
	cr.update_metadata((char*)md.data(), md.md_size);
	cr.init_ccm(2, cimbar::Config::interleave_blocks(), cimbar::Config::interleave_partitions(), cimbar::Config::fountain_chunks_per_frame(6, false));

	assertTrue( decoder._ccm.active() );

	std::stringstream ss;
	ss << decoder._ccm.mat();
	assertEquals("[1.6250746, 0.0024788622, -0.45772526;\n "
				 "-0.29126319, 2.2922182, -0.67037439;\n "
				 "-1.2192062, -2.7447209, 5.0476217]", ss.str());

	std::array<unsigned, 6> expectedColors = {0, 1, 1, 2, 2, 2};
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
