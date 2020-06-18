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

	assertEquals( "56d1cc8c1564e22a63b9e64e686fc67a1f304c693f5513c1fb7f5618816031ba", get_hash(decodedFile) );
}


TEST_CASE( "DecoderTest/testDecodeEcc", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(40);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("4color-ecc40-fountain-0.png"), decodedFile);
	assertEquals( 6900, bytesDecoded );

	assertEquals( "f219b98c8bfaf7369975f7a4e0c95f580308438b6467290a0220b7bb28b807b4", get_hash(decodedFile) );
}
