/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Common.h"

#include "Config.h"
#include "base91/base.hpp"
#include "serialize/format.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <opencv2/opencv.hpp>

#include <map>
#include <string>
#include "bitmaps.h"

using cimbar::RGB;
using std::array;
using std::string;
using std::vector;

namespace {
	RGB getColor4(unsigned index)
	{
		// opencv uses BGR, but we don't have to conform to its tyranny
		static constexpr array<RGB, 4> colors = {
			RGB(0, 0xFF, 0),
			RGB(0, 0xFF, 0xFF),
			RGB(0xFF, 0xFF, 0),
			RGB(0xFF, 0, 0xFF),
		};
		return colors[index];
	}

	RGB getColor4_old(unsigned index)
	{
		static constexpr array<RGB, 4> colors = {
			RGB(0, 0xFF, 0xFF),
			RGB(0xFF, 0xFF, 0),
			RGB(0xFF, 0, 0xFF),
			RGB(0, 0xFF, 0),
		};
		return colors[index];
	}

	RGB getColor8(unsigned index)
	{
		static constexpr array<RGB, 8> colors = {
			RGB(0, 0xFF, 0xFF), // cyan
			RGB(0xFF, 0xFF, 0), // yellow
			RGB(0x7F, 0x7F, 0xFF),  // mid-blue
			RGB(0xFF, 0xFF, 0xFF), // white
			RGB(0, 0xFF, 0), // green
			RGB(0xFF, 0x9F, 0),  // orange
			RGB(0xFF, 0, 0xFF), // magenta
			RGB(0xFF, 65, 65), // red
		};
		return colors[index];
	}

	RGB getColor8_old(unsigned index)
	{
		static constexpr array<RGB, 8> colors = {
			RGB(0, 0xFF, 0xFF), // cyan
			RGB(0x7F, 0x7F, 0xFF),  // mid-blue
			RGB(0xFF, 0, 0xFF), // magenta
			RGB(0xFF, 65, 65), // red
			RGB(0xFF, 0x9F, 0),  // orange
			RGB(0xFF, 0xFF, 0), // yellow
			RGB(0xFF, 0xFF, 0xFF),
			RGB(0, 0xFF, 0),
		};
		return colors[index];
	}
}

namespace cimbar {

cv::Mat load_img(string path)
{
	auto it = cimbar::bitmaps.find(path);
	if (it == cimbar::bitmaps.end())
		return cv::Mat();

	string bytes = base91::decode(it->second);
	vector<unsigned char> data(bytes.data(), bytes.data() + bytes.size());

	int width, height, channels;
	std::unique_ptr<uint8_t[], void (*)(void*)> imgdata(stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, STBI_rgb_alpha), ::free);
	if (!imgdata)
		return cv::Mat();

	size_t len = width * height * channels;
	cv::Mat mat(height, width, CV_MAKETYPE(CV_8U, channels));
	std::copy(imgdata.get(), imgdata.get()+len, mat.data);
	cv::cvtColor(mat, mat, cv::COLOR_RGBA2RGB);
	return mat;
}

RGB getColor(unsigned index, unsigned num_colors, unsigned color_mode)
{
	if (color_mode == 0)
	{
		if (num_colors <= 4)
			return getColor4_old(index);
		else
			return getColor8_old(index);
	}

	if (num_colors <= 4)
		return getColor4(index);
	else
		return getColor8(index);

}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color, unsigned color_mode)
{
	static cv::Vec3b background({0xFF, 0xFF, 0xFF});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath);

	uchar r, g, b;
	std::tie(r, g, b) = getColor(color, num_colors, color_mode);
	cv::MatIterator_<cv::Vec3b> end = tile.end<cv::Vec3b>();
	for (cv::MatIterator_<cv::Vec3b> it = tile.begin<cv::Vec3b>(); it != end; ++it)
	{
		cv::Vec3b& c = *it;
		if (c == background)
		{
			if (dark)
				c = {0, 0, 0};
			continue;
		}
		c = {r, g, b};
	}
	return tile;
}

}
