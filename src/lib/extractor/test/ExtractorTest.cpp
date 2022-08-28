/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

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
	Extractor ext(1024, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_big.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0x2cab639cfa72624, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractMid", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "ex.jpg";
	Extractor ext(1024, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f2_734.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0xc7f8205e686bc02, image_hash::average_hash(out) );
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string imgPath = tempdir.path() / "exup.jpg";
	Extractor ext(1024, 30);
	ext.extract(TestCimbar::getSample("6bit/4_30_f0_627.jpg"), imgPath);

	cv::Mat out = cv::imread(imgPath);
	cv::cvtColor(out, out, cv::COLOR_BGR2RGB);
	assertEquals( 0x29c64eaca3356394, image_hash::average_hash(out) );
}

