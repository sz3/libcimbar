/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "Decoder.h"

#include "compression/zstd_decompressor.h"
#include "fountain/fountain_decoder_sink.h"
#include "util/File.h"
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
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png"), decodedFile);
	assertEquals( 9300, bytesDecoded );

	assertEquals( "7e1919b1210ccc332fc56e8b35cccd622d980f03c6c3b32338bb00aa4b6a22a2", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecodeEcc", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(30);
	std::string decodedFile = tempdir.path() / "testDecode.txt";
	unsigned bytesDecoded = dec.decode(TestCimbar::getSample("6bit/4color_ecc30_fountain_0.png"), decodedFile);
	assertEquals( 7500, bytesDecoded );

	assertEquals( "382c76644a4dff475c5793c5fe061e35e47be252010d29aeaf8d93ee6a3f7045", get_hash(decodedFile) );
}

TEST_CASE( "DecoderTest/testDecodeFountain.Pad", "[unit]" )
{
	MakeTempDirectory tempdir;

	Decoder dec(30);
	fountain_decoder_sink<cimbar::zstd_decompressor<std::ofstream>> fds(tempdir.path(), cimbar::Config::fountain_chunk_size(30));

	cv::Mat img = cv::imread(TestCimbar::getSample("6bit/encoder.fountain_0.png"));
	unsigned bytesDecoded = dec.decode_fountain(img, fds);
	assertEquals( 7500, bytesDecoded );

	std::string contents = File(tempdir.path() / "0.751").read_all();
	assertEquals( "hello", contents );
}

