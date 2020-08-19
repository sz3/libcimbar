
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
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_big.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x2cab6b9cfa72624, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractMid", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("6bit/4_30_f2_734.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0xc5f8225b6c6bc02, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "exup.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_627.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x29c64eac233f6394, image_hash::average_hash(out) );
}

