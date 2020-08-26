/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "encoder/Encoder.h"
#include "fountain/FountainInit.h"
#include "image_hash/average_hash.h"
#include "serialize/format.h"
#include "util/MakeTempDirectory.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>


TEST_CASE( "EncoderTest/testVanilla", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.vanilla";

	Encoder enc(40, 4, 2);
	assertEquals( 3, enc.encode(inputFile, outPrefix) );

	std::vector<uint64_t> hashes = {0xc47ced006f253562, 0x2d8cc44256ccb9eb, 0x6ed0efb800000000};
	for (int i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}_{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}

TEST_CASE( "EncoderTest/testFountain", "[unit]" )
{
	FountainInit::init();
	MakeTempDirectory tempdir;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	Encoder enc(40, 4, 2);
	assertEquals( 5, enc.encode_fountain(inputFile, outPrefix, 0) );

	std::vector<uint64_t> hashes = {0xbb1cc62b662abfe5, 0xf586f6466a5b194, 0x8c2f0f40e6ecb08b, 0x93a3830d042966e1, 0x21f4ef48d3ed685e};
	for (int i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}_{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}

TEST_CASE( "EncoderTest/testFountain.Compress", "[unit]" )
{
	FountainInit::init();
	MakeTempDirectory tempdir;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	Encoder enc(40, 4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix) );

	uint64_t hash = 0xf8cde200e90582e4;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat img = cv::imread(path);
	assertEquals( hash, image_hash::average_hash(img) );
}

TEST_CASE( "EncoderTest/testFountain.Padded", "[unit]" )
{
	FountainInit::init();
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
	cv::Mat img = cv::imread(path);
	assertEquals( hash, image_hash::average_hash(img) );
}
