/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "Decoder.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "DecoderTest/testDecode", "[unit]" )
{
	std::string input = "Hello world";

	Decoder dec;

	unsigned bytesDecoded = dec.decode(get_sample("4.png"), "/tmp/testDecode.txt");
	assertEquals( 9300, bytesDecoded );
}
