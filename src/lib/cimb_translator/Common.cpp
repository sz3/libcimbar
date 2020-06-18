#include "Common.h"

#include "serialize/format.h"

#include "base91/base.hpp"
#include "color-util/RGB_to_XYZ.hpp"
#include "color-util/XYZ_to_Lab.hpp"
#include <opencv2/opencv.hpp>

#include <map>
#include <string>
#include "bitmaps.h"

using std::array;
using std::string;
using std::vector;

namespace {
	array<colorutil::Lab, 8> _convertAllColors()
	{
		array<colorutil::Lab, 8> colors;
		for (int i = 0; i < colors.size(); ++i)
		{
			cimbar::RGB rgb = cimbar::getColor(i);
			colors[i] = cimbar::convertColor(std::get<0>(rgb), std::get<1>(rgb), std::get<2>(rgb));
		}
		return colors;
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

colorutil::Lab convertColor(uchar r, uchar g, uchar b)
{
	colorutil::RGB srgb(r / 255.0, g / 255.0, b / 255.0);
	colorutil::XYZ xyz = colorutil::convert_RGB_to_XYZ(srgb);
	return colorutil::convert_XYZ_to_Lab(xyz);
}

RGB getColor(unsigned index)
{
	// opencv uses BGR, but we don't have to conform to this tyranny
	static constexpr array<RGB, 8> colors = {
	    RGB(0, 0xFF, 0xFF),
	    RGB(0xFF, 0xFF, 0),
	    RGB(0xFF, 0, 0xFF),
	    RGB(0, 0xFF, 0),
	    RGB(0, 0x7F, 0xFF),  // mid-blue
	    RGB(0xFF, 0xFF, 0xFF),
	    RGB(0xFF, 0, 0),
	    RGB(0xFF, 0x7F, 0),  // orange
	};
	return colors[index];
}

colorutil::Lab getLabColor(unsigned index)
{
	static const array<colorutil::Lab, 8> colors = _convertAllColors();
	return colors[index];
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned color, const string& image_dir)
{
	static cv::Vec3b background({0xFF, 0xFF, 0xFF});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath, image_dir);

	uchar r, g, b;
	std::tie(r, g, b) = getColor(color);
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

}
