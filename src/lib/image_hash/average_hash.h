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
		// plenty of room for optimization here...
		// img is assumed to be 8x8 (the 64 bit bitset will explode if it's bigger), but we want to support 9x9 as well)
		// but perhaps with a different function?
		cv::Mat gray;
		if (img.cols != 8 or img.rows != 8)
		{
			cv::Mat resized;
			cv::resize(img, resized, cv::Size(8, 8));
			cv::cvtColor(resized, gray, CV_BGR2GRAY);
		}
		else
			cv::cvtColor(img, gray, CV_BGR2GRAY);

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
