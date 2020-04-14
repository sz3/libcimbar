#include "CimbEncoder.h"

#include "serialize/format.h"
#include <cmath>
#include <iostream>
using cv::Vec3b;
using std::string;

namespace {
	string getTileDir(unsigned symbol_bits)
	{
		return fmt::format("{}/bitmap/{}", LIBCIMBAR_PROJECT_ROOT, symbol_bits);
	}

	Vec3b getColor(unsigned index)
	{
		static const std::array<Vec3b, 8> colors = {
		    Vec3b(0xFF, 0xFF, 0),  // these are stored as BGR, not RGB
		    Vec3b(0, 0xFF, 0xFF),  // don't ask me why
		    Vec3b(0xFF, 0, 0xFF),
		    Vec3b(0, 0xFF, 0),
		    Vec3b(0xFF, 0x7F, 0),
		    Vec3b(0, 0x7F, 0xFF),
		    Vec3b(0, 0, 0xFF),
		    Vec3b(0xFF, 0, 0x7F),
		};
		return colors[index];
	}
}

CimbEncoder::CimbEncoder(unsigned symbol_bits, unsigned color_bits)
    : _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
{
	load_tiles(getTileDir(symbol_bits));
}

cv::Mat CimbEncoder::load_tile(string tile_dir, unsigned index)
{
	unsigned symbol = index % _numSymbols;
	unsigned color = index / _numSymbols;
	Vec3b color3 = getColor(color);
	Vec3b background({0xFF, 0xFF, 0xFF});

	string imgPath = fmt::format("{}/{:02x}.png", tile_dir, symbol);
	cv::Mat tile = cv::imread(imgPath);
	for (int y = 0; y < tile.rows; ++y)
		for (int x = 0; x < tile.cols; ++x)
		{
			Vec3b c = tile.at<Vec3b>(y,x);
			if (c == background)
				continue;
			tile.at<Vec3b>(y,x) = color3;
		}
	return tile;
}

// dir will need to be passed via env? Doesn't make sense to compile it in, and doesn't *really* make sense to use cwd
bool CimbEncoder::load_tiles(std::string tile_dir)
{
	unsigned numTiles = _numColors * _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tiles.push_back(load_tile(tile_dir, i));
	return true;
}

const cv::Mat& CimbEncoder::encode(unsigned bits) const
{
	unsigned symbol = bits % _numSymbols;
	unsigned color = bits / _numSymbols;
	std::cout << fmt::format("symbol {}, color {}", symbol, color) << std::endl;
	return _tiles[symbol];
}
