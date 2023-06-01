/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "average_hash.h"

#include "bit_file/bitbuffer.h"
#include "bit_file/bitmatrix.h"
#include "cimb_translator/CellDrift.h"
#include "cimb_translator/Common.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>

using std::string;

namespace {
	cv::Mat embedTile5x5(const cv::Mat& tile, bool binaryThresh=false)
	{
		cv::Mat sevens(7, 7, tile.type(), cv::Scalar(0, 0, 0));
		tile.copyTo(sevens(cv::Rect(cv::Point(1, 1), tile.size())));

		if (binaryThresh)
		{
			cv::cvtColor(sevens, sevens, cv::COLOR_RGB2GRAY);
			cv::adaptiveThreshold(sevens, sevens, 0xFF, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 3, 0);
		}
		return sevens;
	}

	cv::Mat embedTile8x8(const cv::Mat& tile, bool binaryThresh=false)
	{
		cv::Mat tenxten(10, 10, tile.type(), cv::Scalar(0, 0, 0));
		tile.copyTo(tenxten(cv::Rect(cv::Point(1, 1), tile.size())));

		if (binaryThresh)
		{
			cv::cvtColor(tenxten, tenxten, cv::COLOR_RGB2GRAY);
			cv::adaptiveThreshold(tenxten, tenxten, 0xFF, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 3, 0);
		}
		return tenxten;
	}
}

TEST_CASE( "fuzzyAhashTest/testCorrectness5", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(2, 0, true);
	cv::Mat tenxten = embedTile5x5(tile);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 5, 5);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64)); // we pass in a threshold value to match what fuzzy_ahash will compute
	}

	// do the real work
	auto actual = image_hash::fuzzy_ahash<5>(tenxten);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testCorrectness8", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	cv::Mat tenxten = embedTile8x8(tile);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64)); // we pass in a threshold value to match what fuzzy_ahash will compute
	}

	// do the real work
	auto actual = image_hash::fuzzy_ahash<8>(tenxten);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testIterator", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	cv::Mat tenxten = embedTile8x8(tile);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64)); // we pass in a threshold value to match what fuzzy_ahash will compute
	}

	// do the real work
	image_hash::ahash_result actual = image_hash::fuzzy_ahash<8>(tenxten);
	int count = 0;
	for (auto it : actual)
	{
		++count;
		DYNAMIC_SECTION( "are we correct? : " << it.first )
		{
			assertEquals(it.second, expected[it.first]);
		}
	}
	assertEquals(9, count);

	// different range-based for loop
	count = 0;
	for (auto&& [drift_idx, hash] : actual)
	{
		++count;
		DYNAMIC_SECTION( "2nd for? : " << drift_idx )
		{
			assertEquals(hash, expected[drift_idx]);
		}
	}
	assertEquals(9, count);
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	cv::Mat tenxten = embedTile8x8(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64));
	}

	// do the real work
	auto actual = image_hash::fuzzy_ahash<8>(tenxten, 0xFE);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold.BitMatrix", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	cv::Mat tenxten = embedTile8x8(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64));
	}

	bitbuffer bb;
	bitbuffer::writer writer(bb);
	bitmatrix::mat_to_bitbuffer(tenxten, writer);

	// do the real work
	bitmatrix bm(bb, 10, 10);
	auto actual = image_hash::fuzzy_ahash<8>(bm);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold.BitMatrix8.Fast", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(4, 0, true);
	cv::Mat tenxten = embedTile8x8(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 8, 8);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64));
	}

	bitbuffer bb;
	bitbuffer::writer writer(bb);
	bitmatrix::mat_to_bitbuffer(tenxten, writer);

	// clear the hashes we don't care about
	expected[0] = 0;
	expected[2] = 0;
	expected[6] = 0;
	expected[8] = 0;

	// do the real work
	bitmatrix bm(bb, 10, 10);
	auto actual = image_hash::fuzzy_ahash<8>(bm, image_hash::ahash_result<8>::FAST);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}

TEST_CASE( "fuzzyAhashTest/testPreThreshold.BitMatrix5.Fast", "[unit]" )
{
	cv::Mat tile = cimbar::getTile(2, 0, true);
	cv::Mat tenxten = embedTile5x5(tile, true);

	// compute the hashes we expect
	std::vector<uint64_t> expected;
	for (const std::pair<int, int>& drift : CellDrift::driftPairs)
	{
		cv::Rect crop(drift.first + 1, drift.second + 1, 5, 5);
		cv::Mat img = tenxten(crop);
		expected.push_back(image_hash::average_hash(img, 64));
	}

	bitbuffer bb;
	bitbuffer::writer writer(bb);
	bitmatrix::mat_to_bitbuffer(tenxten, writer);

	// clear the hashes we don't care about
	expected[0] = 0;
	expected[2] = 0;
	expected[6] = 0;
	expected[8] = 0;

	// do the real work
	bitmatrix bm(bb, 7, 7);
	auto actual = image_hash::fuzzy_ahash<5>(bm, image_hash::ahash_result<5>::FAST);

	for (unsigned i = 0; i < actual.size(); ++i)
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			assertEquals(expected[i], actual[i]);
		}
}
