
#include "unittest.h"

#include "Extractor.h"
#include "image_hash/average_hash.h"
#include "util/MakeTempDirectory.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "ExtractorTest/testExtract", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("4_30_f0_big.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0xf05a56726c5e5400, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractMid", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("4_30_f2_734.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x2c57aa25f6d7b802, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "exup.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("4_30_f0_627.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x29c64eaca3376394, image_hash::average_hash(out) );
}

