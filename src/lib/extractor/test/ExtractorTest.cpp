#include "unittest.h"

#include "Extractor.h"
#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "ExtractorTest/testExtract", "[unit]" )
{
	Extractor ext;
	ext.extract(TestCimbar::getSample("4color1.jpg"), "/tmp/testExtract.1.jpg");

	// standard image similarity testing questions apply
}

TEST_CASE( "ExtractorTest/testExtractUpscale", "[unit]" )
{
	Extractor ext;
	ext.extract(TestCimbar::getSample("4color-cam-140.jpg"), "/tmp/testExtract.2.jpg");

	// standard image similarity testing questions apply
}

