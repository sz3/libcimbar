/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "Decoder.h"
#include "PicoSHA2/picosha2.h"

#include <fstream>
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
	Decoder dec;
	unsigned bytesDecoded = dec.decode(get_sample("4.png"), "/tmp/testDecode.txt");
	assertEquals( 9300, bytesDecoded );

	std::ifstream f("/tmp/testDecode.txt", std::ios::binary);
	std::vector<unsigned char> hash(picosha2::k_digest_size);
	picosha2::hash256(f, hash.begin(), hash.end());
	assertEquals( "0208d038b2b31417b2c0773bbeb467ca2e37db87e7c50b309e72049149da4e01", picosha2::bytes_to_hex_string(hash) );
}


TEST_CASE( "DecoderTest/testDecodeMessy", "[unit]" )
{
	Decoder dec;
	unsigned bytesDecoded = dec.decode(get_sample("4color1e.png"), "/tmp/testDecode2.txt");
	assertEquals( 9300, bytesDecoded );
}
