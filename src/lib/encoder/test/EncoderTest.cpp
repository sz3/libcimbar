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
			std::string path = fmt::format("{}-{}.png", outPrefix, i);
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
	assertEquals( 5, enc.encode_fountain(inputFile, outPrefix) );

	std::vector<uint64_t> hashes = {0xbb1cc62b662abfe5, 0xf586f6466a5b194, 0x8c2f0f40e6ecb08b, 0x93a3830d042966e1, 0x21f4ef48d3ed685e};
	for (int i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}-{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}
