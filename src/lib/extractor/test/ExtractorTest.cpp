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
	ext.extract(TestCimbar::getSample("4color1.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x4b4caa8597fc56c7, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractMid", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("4c-cam-40-f1.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0xe0d2b532f6b9b270, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "exup.jpg";
	Extractor ext;
	ext.extract(TestCimbar::getSample("4color-cam-140.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	assertEquals( 0x204cc7f08dfd8f5c, image_hash::average_hash(out) );
}

