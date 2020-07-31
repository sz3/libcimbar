#include "Common.h"

#include "serialize/format.h"

#include "base91/base.hpp"
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
		// opencv uses BGR, but we don't have to conform to this tyranny
		static constexpr array<RGB, 4> colors = {
		    RGB(0, 0xFF, 0xFF),
		    RGB(0xFF, 0xFF, 0),
		    RGB(0xFF, 0, 0xFF),
		    RGB(0, 0xFF, 0)
		};
		return colors[index];
	}

	RGB getColor8(unsigned index)
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

cv::Mat load_img(string path, const string& image_dir)
{
	if (image_dir != "")
		return cv::imread(image_dir + "/" + path);

	auto it = cimbar::bitmaps.find(path);
	if (it == cimbar::bitmaps.end())
		return cv::imread(path);

	string bytes = base91::decode(it->second);
	vector<char> data(bytes.data(), bytes.data() + bytes.size());
	return cv::imdecode(data, cv::IMREAD_COLOR);
}

RGB getColor(unsigned index, unsigned num_colors)
{
	if (num_colors <= 4)
		return getColor4(index);
	else
		return getColor8(index);
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned num_colors, unsigned color, const string& image_dir)
{
	static cv::Vec3b background({0xFF, 0xFF, 0xFF});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath, image_dir);

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
			continue;
		}
		c = {b, g, r};
	}
	return tile;
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, const string& image_dir, unsigned num_colors, unsigned color)
{
	return getTile(symbol_bits, symbol, dark, num_colors, color, image_dir);
}

}
