#pragma once

#include "serialize/format.h"

#include "bitset_extractor.h"

#include <opencv2/opencv.hpp>

#include <bitset>
#include <cstdint>
#include <vector>

namespace image_hash
{
	inline uint64_t average_hash(const cv::Mat& img)
	{
		cv::Mat gray = img;
		if (img.channels() != 1)
			cv::cvtColor(gray, gray, cv::COLOR_BGR2GRAY);
		if (gray.cols != 8 or gray.rows != 8)
			cv::resize(gray, gray, cv::Size(8, 8));

		unsigned total = 0;
		unsigned count = 64;

		//cv::Scalar myMatMean = cv::mean(gray);
		//unsigned char avg = (unsigned char)myMatMean[0];

		cv::MatIterator_<uchar> end = gray.end<uchar>();
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it)
			total += *it;
		unsigned char avg = total / count;

		std::bitset<64> res;
		unsigned i = 0;
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it, ++i)
		{
			if (*it > avg)
				res.set(i);
		}
		return res.to_ullong();
	}

	// need something like a bitset_extractor(), with an api like:
	// bits.extract(0, 8,  9, 17,  18, 26,  27, 35,  36, 44,  45, 53,  54, 62,  63, 71)
	//  ... probably using template magic. For our purposes, we need >= 8 pairs of bit indices, which will sum to a total of 64 bits
	// or maybe a better api is:
	// bits.extract(0, 9, 18, 27, 36, 45, 54, 63)
	//  ... with each index corresponding to an 8 bit read?
	//  ... this way, we could do compile time validation that the return value makes sense.
	//  ... e.g. if we have 8 params, that means it's a 64 bit number being returned.
	inline std::vector<uint64_t> fuzzy_ahash(const cv::Mat& img)
	{
		// return 9 uint64_ts, each representing an 8x8 section of the 10x10 img
		cv::Mat gray = img;
		if (img.channels() != 1)
			cv::cvtColor(gray, gray, cv::COLOR_BGR2GRAY);
		if (gray.cols != 10 or gray.rows != 10)
			cv::resize(gray, gray, cv::Size(10, 10));

		unsigned total = 0;
		unsigned count = 100;

		cv::MatIterator_<uchar> end = gray.end<uchar>();
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it)
			total += *it;
		unsigned char avg = total / count;

		std::bitset<100> res;
		unsigned i = 0;
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it, ++i)
		{
			if (*it > avg)
				res.set(i);
		}

		// quick test {0, 0}, {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}

		bitset_extractor<100> be(res);
		std::vector<uint64_t> hashes;
		// middle
		hashes.push_back(be.extract(11, 21, 31, 41, 51, 61, 71, 81));
		hashes.push_back(be.extract(12, 22, 32, 42, 52, 62, 72, 82));
		hashes.push_back(be.extract(21, 31, 41, 51, 61, 71, 81, 91));
		hashes.push_back(be.extract(10, 20, 30, 40, 50, 60, 70, 80));
		hashes.push_back(be.extract(1, 11, 21, 31, 41, 51, 61, 71));
		// edges
		hashes.push_back(be.extract(22, 32, 42, 52, 62, 72, 82, 92));
		hashes.push_back(be.extract(0, 10, 20, 30, 40, 50, 60, 70));
		hashes.push_back(be.extract(2, 12, 22, 32, 42, 52, 62, 72));
		hashes.push_back(be.extract(20, 30, 40, 50, 60, 70, 80, 90));
		return hashes;
	}
}
