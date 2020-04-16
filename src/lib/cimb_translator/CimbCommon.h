#pragma once

#include "serialize/format.h"
#include <opencv2/opencv.hpp>
#include <string>

namespace cimbar
{
	inline cv::Vec3b getColor(unsigned index)
	{
		static const std::array<cv::Vec3b, 8> colors = {
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

	inline std::string getTileDir(unsigned symbol_bits)
	{
		return fmt::format("{}/bitmap/{}", LIBCIMBAR_PROJECT_ROOT, symbol_bits);
	}

	inline cv::Mat getTile(std::string tile_dir, unsigned symbol, bool dark=true, unsigned color=0)
	{
		static cv::Vec3b background({0xFF, 0xFF, 0xFF});

		std::string imgPath = fmt::format("{}/{:02x}.png", tile_dir, symbol);
		cv::Mat tile = cv::imread(imgPath);

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
