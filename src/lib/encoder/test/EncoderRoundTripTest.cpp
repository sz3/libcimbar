/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "encoder/Decoder.h"
#include "encoder/EncoderPlus.h"

#include "compression/zstd_decompressor.h"
#include "fountain/fountain_decoder_sink.h"
#include "image_hash/average_hash.h"
#include "serialize/format.h"
#include "util/ConfigScope.h"
#include "util/File.h"
#include "util/MakeTempDirectory.h"

#include <iostream>
#include <string>

namespace {
	// evil hacks for MSVC
	std::string operator/(const std::filesystem::path& lhs, const char* rhs)
	{
		return (lhs / std::filesystem::path(rhs)).string();
	}

	std::string operator/(const std::filesystem::path& lhs, const std::string& rhs)
	{
		return (lhs / std::filesystem::path(rhs)).string();
	}
}

TEST_CASE( "EncoderRoundTripTest/testFountain.Pad", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string inputFile = tempdir.path() / "hello.txt";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	{
		std::ofstream f(inputFile, std::ios::binary);
		f << "hello"; // 5 bytes!
	}

	// will be padded so the fountain encoding is happy. The encoded image looks suspiciously non-random!
	EncoderPlus enc(4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix) );

	uint64_t hash = 0x67df46e0c285e20e;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat encodedImg = cv::imread(path);
	cv::cvtColor(encodedImg, encodedImg, cv::COLOR_BGR2RGB);
	assertEquals( hash, image_hash::average_hash(encodedImg) );

	SECTION ("default_filename") {
		// decoder
		Decoder dec;
		fountain_decoder_sink fds(cimbar::Config::fountain_chunk_size(), write_on_store<cimbar::zstd_decompressor<std::ofstream>>(tempdir.path().string()));

		unsigned bytesDecoded = dec.decode_fountain(encodedImg, fds);
		assertEquals( 7500, bytesDecoded );

		std::string decodedContents = File(tempdir.path() / "0.626").read_all();
		assertEquals( "hello", decodedContents );

		assertEquals( 1, fds.num_done() );
	}

	SECTION ("parsed_filename") {
		Decoder dec;
		fountain_decoder_sink fds(cimbar::Config::fountain_chunk_size(), decompress_on_store<std::ofstream>(tempdir.path().string()));

		unsigned bytesDecoded = dec.decode_fountain(encodedImg, fds);
		assertEquals( 7500, bytesDecoded );

		std::string decodedContents = File(tempdir.path() / "hello.txt").read_all();
		assertEquals( "hello", decodedContents );

		assertEquals( 1, fds.num_done() );
	}
}

TEST_CASE( "EncoderRoundTripTest/testFountain.SinkMismatch", "[unit]" )
{
	MakeTempDirectory tempdir;
	ConfigScope cs(4);

	std::string inputFile = tempdir.path() / "hello.txt";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	{
		std::ofstream f(inputFile, std::ios::binary);
		f << "hello"; // 5 bytes!
	}

	// will be padded so the fountain encoding is happy. The encoded image looks suspiciously non-random!
	EncoderPlus enc(4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix) );

	uint64_t hash = 0xa617c720384386cb;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat encodedImg = cv::imread(path);
	cv::cvtColor(encodedImg, encodedImg, cv::COLOR_BGR2RGB);
	assertEquals( hash, image_hash::average_hash(encodedImg) );

	// decoder
	Decoder dec;
	// sink with a mismatched fountain_chunk_size
	// importantly, the sink expects a *smaller* chunk than we'll give it...
	// because that's a more interesting test...
	fountain_decoder_sink fds(cimbar::Config::fountain_chunk_size()-125, write_on_store<cimbar::zstd_decompressor<std::ofstream>>(tempdir.path().string()));

	unsigned bytesDecoded = dec.decode_fountain(encodedImg, fds);
	assertEquals( 7500, bytesDecoded );

	assertEquals( 0, fds.num_done() );
}

TEST_CASE( "EncoderRoundTripTest/testStreaming", "[unit]" )
{
	MakeTempDirectory tempdir;

	//input
	std::ifstream infile(TestCimbar::getProjectDir() + "/LICENSE");

	// create encoder
	EncoderPlus enc(4, 2);
	fountain_encoder_stream::ptr fes = enc.create_fountain_encoder(infile, "");
	assertTrue( fes );
	assertTrue( fes->good() );

	// create decoder
	Decoder dec;
	fountain_decoder_sink fds(cimbar::Config::fountain_chunk_size(), write_on_store<cimbar::zstd_decompressor<std::ofstream>>(tempdir.path().string()));

	// encode frames, then pass to decoder
	for (int i = 0; i < 100; ++i)
	{
		std::optional<cv::Mat> frame = enc.encode_next(*fes);
		assertTrue( frame );

		unsigned bytesDecoded = dec.decode_fountain(*frame, fds);
		assertEquals( 7500, bytesDecoded );

		if (fds.num_done())
			break;
	}

	// done
	assertEquals( 1, fds.num_done() );
	std::string decodedContents = File(tempdir.path() / "0.5256").read_all();
	assertEquals( 16727, decodedContents.size() );
	assertStringContains( "Mozilla Public License Version 2.0", decodedContents );
}
