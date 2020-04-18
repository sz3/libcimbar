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

cv::Vec3b getColor(unsigned index)
{
	static const array<cv::Vec3b, 8> colors = {
	    cv::Vec3b(0xFF, 0xFF, 0),  // these are stored as BGR, not RGB
	    cv::Vec3b(0, 0xFF, 0xFF),  // feels bad man
	    cv::Vec3b(0xFF, 0, 0xFF),
	    cv::Vec3b(0, 0xFF, 0),
	    cv::Vec3b(0xFF, 0x7F, 0),
	    cv::Vec3b(0, 0x7F, 0xFF),
	    cv::Vec3b(0, 0, 0xFF),
	    cv::Vec3b(0xFF, 0, 0x7F),
	};
	return colors[index];
}

cv::Mat getTile(unsigned symbol_bits, unsigned symbol, bool dark, unsigned color, const string& image_dir)
{
	static cv::Vec3b background({0xFF, 0xFF, 0xFF});

	string imgPath = fmt::format("bitmap/{}/{:02x}.png", symbol_bits, symbol);
	cv::Mat tile = load_img(imgPath, image_dir);

	cv::Vec3b color3 = getColor(color);
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
		c = color3;
	}
	return tile;
}

}
