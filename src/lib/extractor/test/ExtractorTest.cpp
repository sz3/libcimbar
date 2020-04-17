#include "unittest.h"

#include "Extractor.h"
#include <iostream>
#include <string>
#include <vector>

namespace
{
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "ExtractorTest/testExtract", "[unit]" )
{
	Extractor ext;
	ext.extract(get_sample("4color1.jpg"), "/tmp/testExtract.1.jpg");

	// standard image similarity testing questions apply
}

