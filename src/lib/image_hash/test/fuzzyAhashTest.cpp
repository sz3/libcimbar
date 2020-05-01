#include "unittest.h"

#include "average_hash.h"

#include "cimb_translator/CellDrift.h"
#include "cimb_translator/Common.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

using std::string;

TEST_CASE( "fuzzyAhashTest/testCorrectness", "[unit]" )
{
	string dir = TestCimbar::getProjectDir();
	cv::Mat tile = cimbar::getTile(4, 0, true, 0, dir);
	cv::resize(tile, tile, cv::Size(10, 10));

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tile(crop);
		expected.push_back(image_hash::average_hash(img, 100)); // we pass in a threshold value to match what fuzzy_ahash will compute
	}

	// do the real work
	std::vector<uint64_t> actual = image_hash::fuzzy_ahash(tile);

	for (int i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}
