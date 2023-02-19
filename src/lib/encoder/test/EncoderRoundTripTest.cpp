/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "encoder/Decoder.h"
#include "encoder/Encoder.h"

#include "compression/zstd_decompressor.h"
#include "fountain/fountain_decoder_sink.h"
#include "image_hash/average_hash.h"
#include "serialize/format.h"
#include "util/File.h"
#include "util/MakeTempDirectory.h"

#include <iostream>
#include <string>

TEST_CASE( "EncoderRoundTripTest/testFountain.Pad", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string inputFile = tempdir.path() / "hello.txt";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	{
		std::ofstream f(inputFile);
		f << "hello"; // 5 bytes!
	}

	// will be padded so the fountain encoding is happy. The encoded image looks suspiciously non-random!
	Encoder enc(30, 4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix) );

	uint64_t hash = 0xaecc8c00efce8c28;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat encodedImg = cv::imread(path);
	cv::cvtColor(encodedImg, encodedImg, cv::COLOR_BGR2RGB);
	assertEquals( hash, image_hash::average_hash(encodedImg) );

	// decoder
	Decoder dec(30);
	fountain_decoder_sink<cimbar::zstd_decompressor<std::ofstream>> fds(tempdir.path(), cimbar::Config::fountain_chunk_size(30));

	unsigned bytesDecoded = dec.decode_fountain(encodedImg, fds);
	assertEquals( 7500, bytesDecoded );

	std::string decodedContents = File(tempdir.path() / "0.751").read_all();
	assertEquals( "hello", decodedContents );
}

TEST_CASE( "EncoderRoundTripTest/testStreaming", "[unit]" )
{
	MakeTempDirectory tempdir;

	//input
	std::ifstream infile(TestCimbar::getProjectDir() + "/LICENSE");

	// create encoder
	Encoder enc(30, 4, 2);
	fountain_encoder_stream::ptr fes = enc.create_fountain_encoder(infile);
	assertTrue( fes );
	assertTrue( fes->good() );

	// create decoder
	Decoder dec(30);
	fountain_decoder_sink<cimbar::zstd_decompressor<std::ofstream>> fds(tempdir.path(), cimbar::Config::fountain_chunk_size(30));

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
