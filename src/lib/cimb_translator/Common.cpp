/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Common.h"

#include "graphics/res.h"
#include "serialize/format.h"
// if no raylib, define stb image?
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <opencv2/opencv.hpp>

using cimbar::RGB;
using std::string;

namespace cimbar {

cv::Mat load_img(string path)
{
	string bytes = load_file(path);
	if (bytes.empty())
		return cv::Mat();

	int width, height, channels;
	std::unique_ptr<uint8_t[]> imgdata(stbi_load_from_memory(reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels, STBI_rgb_alpha));
	size_t len = width * height * channels;
	cv::Mat mat(height, width, CV_MAKETYPE(CV_8U, channels));
	std::copy(imgdata.get(), imgdata.get()+len, mat.data);
	cv::cvtColor(mat, mat, cv::COLOR_RGBA2RGB);
	return mat;
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color)
{
	static cv::Vec3b background({0, 0, 0});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath);

	uchar r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors);
	cv::MatIterator_<cv::Vec3b> end = tile.end<cv::Vec3b>();
	for (cv::MatIterator_<cv::Vec3b> it = tile.begin<cv::Vec3b>(); it != end; ++it)
	{
		cv::Vec3b& c = *it;
		if (c == background)
		{
			if (dark)
				c = {0, 0, 0};
			else
				c = {255, 255, 255};
			continue;
		}
		c = {r, g, b};
	}
	return tile;
}

}
