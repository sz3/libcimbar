/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "ahash_result.h"
#include "bit_extractor.h"
#include "bit_file/bitmatrix.h"
#include "cimb_translator/Cell.h"

#include "intx/intx.hpp"
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
			cv::cvtColor(gray, gray, cv::COLOR_RGB2GRAY);

		if (threshold == 0)
			threshold = Cell(gray).mean_grayscale();

		uint64_t res = 0;
		int bitpos = gray.rows*gray.cols - 1; // ex: 8*8 - 1
		for (int i = 0; i < gray.rows; ++i)
		{
			const uchar* p = gray.ptr<uchar>(i);
			for (int j = 0; j < gray.cols; ++j, --bitpos)
				res |= (uint64_t)(p[j] > threshold) << bitpos;
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
	inline ahash_result fuzzy_ahash(const cv::Mat& img, uchar threshold=0, unsigned mode=ahash_result::ALL)
	{
		// return 9 uint64_ts, each representing an 5x5 section of the 7x7 img
		cv::Mat gray = img;
		if (img.channels() != 1)
			cv::cvtColor(gray, gray, cv::COLOR_RGB2GRAY);
		if (gray.cols > 8 or gray.rows > 8)
			cv::resize(gray, gray, cv::Size(8, 8));

		if (threshold == 0)
			threshold = Cell(gray).mean_grayscale();

		uint64_t res(0);
		int bitpos = gray.cols*gray.rows - 1; // 8*8 - 1
		for (int i = 0; i < gray.rows; ++i)
		{
			const uchar* p = gray.ptr<uchar>(i);
			for (int j = 0; j < gray.cols; ++j, --bitpos)
				res |= uint64_t(p[j] > threshold) << bitpos;
		}
		return ahash_result(res, mode);
	}

	inline ahash_result fuzzy_ahash(const bitmatrix& img, unsigned mode=ahash_result::ALL)
	{
		uint64_t res(0);
		int bitpos = 42; // 7*7 - 7 ... TODO: get from img somehow?
		for (unsigned i = 0; i < 7; ++i, bitpos-=7)
		{
			uint64_t r = img.get(0, i, 7);
			res |= r << bitpos;
		}
		return ahash_result(res, mode);
	}
}
