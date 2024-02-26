/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

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
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("b/tr_0.png"), decodedFile);
	assertEquals( 9300, bytesDecoded );

	assertEquals( "ddcb6cd47751df1402dcf2cffdace212bc9e4a4b6ef097ad4828913086309469", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecodeEcc", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(30);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("b/tr_0.png"), decodedFile);
	assertEquals( 7500, bytesDecoded );

	assertEquals( "a0e9fff8cd5b13807fae215b8b07e38091d3f533ff46243b53ee7f74fbbee0d5", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecode.Sample", "[unit]" )
{
	// regression test -- useful for now, but is very brittle
	MakeTempDirectory tempdir;

	Decoder dec(0);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("b/scan2434.jpg"), decodedFile);
	assertEquals( 9300, bytesDecoded );

	if (CV_VERSION_MAJOR == 4)
		assertEquals( "ccb39ac3511a8974a8e98d3ea321d576d974d28f0b9373fc611ce6a4c83b561c", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecode.4c", "[unit]" )
{
	// legacy format
	MakeTempDirectory tempdir;

	Decoder dec(0, 2, true);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png"), decodedFile, 0);
	assertEquals( 9300, bytesDecoded );

	assertEquals( "7e1919b1210ccc332fc56e8b35cccd622d980f03c6c3b32338bb00aa4b6a22a2", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecodeEcc.4c", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(30, 2, true);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png"), decodedFile, 0);
	assertEquals( 7500, bytesDecoded );

	assertEquals( "382c76644a4dff475c5793c5fe061e35e47be252010d29aeaf8d93ee6a3f7045", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecode.Sample4c", "[unit]" )
{
	// regression test -- useful for now, but is very brittle
	MakeTempDirectory tempdir;

	Decoder dec(0, 2, true);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("6bit/4_30_f0_627_extract.jpg"), decodedFile, 0);
	assertEquals( 9300, bytesDecoded );

	if (CV_VERSION_MAJOR == 4)
		assertEquals( "2040c157884c476def842f7854621a7655182e5f11a34ade563616d93cb93455", get_hash(decodedFile) );
}
