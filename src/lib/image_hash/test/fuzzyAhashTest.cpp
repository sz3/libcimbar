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
	auto bits = image_hash::fuzzy_ahash(tile);
	std::array<uint64_t,9> actual = image_hash::extract_fuzzy_ahash(bits);

	for (int i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold", "[unit]" )
{
	string dir = TestCimbar::getProjectDir();
	cv::Mat tile = cimbar::getTile(4, 0, true, 0, dir);
	cv::resize(tile, tile, cv::Size(10, 10));
	cv::cvtColor(tile, tile, cv::COLOR_BGR2GRAY);
	cv::adaptiveThreshold(tile, tile, 0xFF, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 0);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tile(crop);
		expected.push_back(image_hash::average_hash(img, 100));
	}

	// do the real work
	auto bits = image_hash::fuzzy_ahash(tile, 0xFE);
	std::array<uint64_t,9> actual = image_hash::extract_fuzzy_ahash(bits);

	for (int i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}
