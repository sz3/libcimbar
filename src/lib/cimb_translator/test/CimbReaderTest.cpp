#include "unittest.h"

#include "CimbReader.h"

#include "cimb_translator/Common.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
using std::string;

namespace {
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "CimbReaderTest/testSample", "[unit]" )
{
	string sample_path = get_sample("4.png");

	CimbReader cr(sample_path);

	// read
	unsigned bits1 = cr.read();
	assertEquals(8, bits1);

	unsigned bits2 = cr.read();
	assertEquals(50, bits2);

	unsigned bits3 = cr.read();
	assertEquals(4, bits3);

	unsigned bits4 = cr.read();
	assertEquals(47, bits4);

	while (!cr.done())
		cr.read();
	assertTrue(cr.done());
}

TEST_CASE( "CimbReaderTest/testSampleMessy", "[unit]" )
{
	string sample_path = get_sample("4color1e.png");

	CimbReader cr(sample_path);

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
