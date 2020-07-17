#include "unittest.h"

#include "average_hash.h"

#include "cimb_translator/CellDrift.h"
#include "cimb_translator/Common.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

using std::string;

namespace {
	cv::Mat embedTile(const cv::Mat& tile, bool binaryThresh=false)
	{
		cv::Mat tenxten(10, 10, tile.type());
		tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));

		if (binaryThresh)
		{
			cv::cvtColor(tenxten, tenxten, cv::COLOR_BGR2GRAY);
			cv::adaptiveThreshold(tenxten, tenxten, 0xFF, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 0);
		}
		return tenxten;
	}
}

TEST_CASE( "fuzzyAhashTest/testCorrectness", "[unit]" )
{
	string dir = TestCimbar::getProjectDir();
	cv::Mat tile = cimbar::getTile(4, 0, true, 0, dir);
	cv::Mat tenxten = embedTile(tile);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 100)); // we pass in a threshold value to match what fuzzy_ahash will compute
	}

	// do the real work
	auto bits = image_hash::fuzzy_ahash(tenxten);
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
	cv::Mat tenxten = embedTile(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 100));
	}

	// do the real work
	auto bits = image_hash::fuzzy_ahash(tenxten, 0xFE);
	std::array<uint64_t,9> actual = image_hash::extract_fuzzy_ahash(bits);

	for (int i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold.SpecialCase", "[unit]" )
{
	string dir = TestCimbar::getProjectDir();
	cv::Mat tile = cimbar::getTile(4, 0, true, 0, dir);
	cv::Mat tenxten = embedTile(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 100));
	}

	// do the real work
	// 0xFF is a special case. It says we *only* care about 0xFF values,
	// e.g. we can make some assumptions about the input data that might help us optimize...
	auto bits = image_hash::fuzzy_ahash(tenxten, 0xFF);
	std::array<uint64_t,9> actual = image_hash::extract_fuzzy_ahash(bits);

	for (int i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}
