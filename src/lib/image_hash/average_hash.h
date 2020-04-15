#pragma once

#include "serialize/format.h"

#include <opencv2/opencv.hpp>

#include <bitset>
#include <cstdint>

namespace image_hash
{
	uint64_t average_hash(const cv::Mat& img)
	{
		// plenty of room for optimization here...
		// img is assumed to be 8x8 (the 64 bit bitset will explode if it's bigger), but we want to support 9x9 as well)
		// but perhaps with a different function?

		cv::Mat gray;
		cv::cvtColor(img, gray, CV_BGR2GRAY);
		unsigned total = 0;
		unsigned count = gray.rows * gray.cols;

		//cv::Scalar myMatMean = cv::mean(gray);

		cv::MatIterator_<uchar> end = gray.end<uchar>();
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it)
			total += *it;
		unsigned char avg = total / count;

		std::bitset<64> res; // TODO: no
		unsigned i = 0;
		for (cv::MatIterator_<uchar> it = gray.begin<uchar>(); it != end; ++it, ++i)
		{
			total += *it;
			if (*it > avg)
				res.set(i);
		}
		return res.to_ullong();
	}
}
