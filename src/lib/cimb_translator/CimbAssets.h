#pragma once

#include "bitmaps.h"

#include "base91/base.hpp"
#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

namespace CimbAssets
{
	static inline cv::Mat load_img(std::string path, const std::string& image_dir="")
	{
		if (image_dir != "")
			return cv::imread(image_dir + "/" + path);

		auto it = cimbar::bitmaps.find(path);
		if (it == cimbar::bitmaps.end())
			return cv::imread(path);

		std::string bytes = base91::decode(it->second);
		std::vector<char> data(bytes.data(), bytes.data() + bytes.size());
		return cv::imdecode(data, cv::IMREAD_COLOR);
	}
}
