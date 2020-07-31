/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "Decoder.h"

#include "util/MakeTempDirectory.h"
#include "PicoSHA2/picosha2.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
	std::string get_hash(std::string filename)
	{
		std::ifstream f(filename, std::ios::binary);
		std::vector<unsigned char> hash(picosha2::k_digest_size);
		picosha2::hash256(f, hash.begin(), hash.end());
		return picosha2::bytes_to_hex_string(hash);
	}
}

TEST_CASE( "DecoderTest/testDecode", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(0);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("4color-ecc40-fountain-0.png"), decodedFile);
	assertEquals( 9300, bytesDecoded );

	assertEquals( "2ca63e05281d7aeeed2be469b6872b4357b94be1ef02fe9dba5d1736d6ae64cc", get_hash(decodedFile) );
}


TEST_CASE( "DecoderTest/testDecodeEcc", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(40);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("4color-ecc40-fountain-0.png"), decodedFile);
	assertEquals( 6900, bytesDecoded );

	assertEquals( "c7a1574e6a40ea446cb3cf40bfb346ab7cdc2a5d3b1c59908e676c466a3285a1", get_hash(decodedFile) );
}
