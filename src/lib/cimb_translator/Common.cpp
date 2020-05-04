#include "Common.h"

#include "serialize/format.h"

#include "base91/base.hpp"
#include <opencv2/opencv.hpp>

#include <map>
#include <string>
#include "bitmaps.h"

using std::array;
using std::string;
using std::vector;

namespace cimbar {

using RGB = std::tuple<uchar,uchar,uchar>;

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

RGB getColor(unsigned index)
{
	// opencv uses BGR, but we don't have to conform to this tyranny
	static const array<RGB, 8> colors = {
	    RGB(0, 0xFF, 0xFF),
	    RGB(0xFF, 0xFF, 0),
	    RGB(0xFF, 0, 0xFF),
	    RGB(0, 0xFF, 0),
	    RGB(0, 0x7F, 0xFF),
	    RGB(0xFF, 0x7F, 0),
	    RGB(0xFF, 0, 0),
	    RGB(0x7F, 0, 0xFF),
	};
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
