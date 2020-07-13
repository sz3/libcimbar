#pragma once

#include "bit_extractor.h"
#include "cimb_translator/Cell.h"

#include "intx/int128.hpp"
#include <opencv2/opencv.hpp>

#include <array>
#include <bitset>
#include <cstdint>

namespace image_hash
{
	inline uint64_t average_hash(const cv::Mat& img, uchar threshold=0)
	{
		cv::Mat gray = img;
		if (img.channels() != 1)
			cv::cvtColor(gray, gray, cv::COLOR_BGR2GRAY);
		if (gray.cols != 8 or gray.rows != 8)
			cv::resize(gray, gray, cv::Size(8, 8));

		if (threshold == 0)
			threshold = Cell(gray).mean_grayscale();

		uint64_t res = 0;
		int count = 0;
		for (int i = 0; i < gray.rows; ++i)
		{
			const uchar* p = gray.ptr<uchar>(i);
			for (int j = 0; j < gray.cols; ++j, ++count)
				res |= (uint64_t)(p[j] > threshold) << count;
		}
		return res;
	}

	// need something like a bitset_extractor(), with an api like:
	// bits.extract(0, 8,  9, 17,  18, 26,  27, 35,  36, 44,  45, 53,  54, 62,  63, 71)
	//  ... probably using template magic. For our purposes, we need >= 8 pairs of bit indices, which will sum to a total of 64 bits
	// or maybe a better api is:
	// bits.extract(0, 9, 18, 27, 36, 45, 54, 63)
	//  ... with each index corresponding to an 8 bit read?
	//  ... this way, we could do compile time validation that the return value makes sense.
	//  ... e.g. if we have 8 params, that means it's a 64 bit number being returned.
	inline intx::uint128 fuzzy_ahash(const cv::Mat& img, uchar threshold=0)
	{
		// return 9 uint64_ts, each representing an 8x8 section of the 10x10 img
		cv::Mat gray = img;
		if (img.channels() != 1)
			cv::cvtColor(gray, gray, cv::COLOR_BGR2GRAY);
		if (gray.cols != 10 or gray.rows != 10)
			cv::resize(gray, gray, cv::Size(10, 10));

		if (threshold == 0)
			threshold = Cell(gray).mean_grayscale();

		intx::uint128 res(0);
		int count = 0;
		for (int i = 0; i < gray.rows; ++i)
		{
			const uchar* p = gray.ptr<uchar>(i);
			for (int j = 0; j < gray.cols; ++j, ++count)
				res |= intx::uint128(p[j] > threshold) << count;
		}
		return res;
	}

	inline std::array<uint64_t, 9> extract_fuzzy_ahash(const intx::uint128& bits)
	{
		bit_extractor<intx::uint128, 100> be(bits);
		std::array<uint64_t, 9> hashes = {
		    // top row -- top left bit is the end bit. bottom right is 0.
		    be.extract(22, 32, 42, 52, 62, 72, 82, 92),  // left
		    be.extract(21, 31, 41, 51, 61, 71, 81, 91),
		    be.extract(20, 30, 40, 50, 60, 70, 80, 90),  // right
		    // middle row
		    be.extract(12, 22, 32, 42, 52, 62, 72, 82),
		    be.extract(11, 21, 31, 41, 51, 61, 71, 81),
		    be.extract(10, 20, 30, 40, 50, 60, 70, 80),
		    // bottom row
		    be.extract(2, 12, 22, 32, 42, 52, 62, 72),
		    be.extract(1, 11, 21, 31, 41, 51, 61, 71),
		    be.extract(0, 10, 20, 30, 40, 50, 60, 70)
		};
		return hashes;
	}
}
