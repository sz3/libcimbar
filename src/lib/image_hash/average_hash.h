#pragma once

#include "serialize/format.h"

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

		cv::MatIterator_<uchar> end = gray.end<uchar>();
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it)
			total += *it;
		unsigned char avg = total / count;

		std::bitset<64> res;
		unsigned i = 0;
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it, ++i)
		{
			total += *it;
			if (*it > avg)
				res.set(i);
		}
		return res.to_ullong();
	}

	inline std::vector<uint64_t> fuzzy_hash(const cv::Mat& img)
	{
		// return 9 uint64_ts, each representing an 8x8 section of the 9x9 img
		std::vector<uint64_t> hashes;
		return hashes;
	}
}
