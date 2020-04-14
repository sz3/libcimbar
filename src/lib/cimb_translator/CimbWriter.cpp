#include "CimbWriter.h"

#include "serialize/format.h"
#include <cmath>
#include <iostream>
using std::string;

namespace {
	string getTileDir(unsigned tile_bits)
	{
		return fmt::format("{}/bitmap/{}", LIBCIMBAR_PROJECT_ROOT, tile_bits);
	}
}

CimbWriter::CimbWriter(unsigned tile_bits)
{
	_numSymbols = (1 << tile_bits);
	load_tiles(getTileDir(tile_bits));
}

cv::Mat CimbWriter::load_tile(string tile_dir, unsigned index)
{
	string imgPath = fmt::format("{}/{:02x}.png", tile_dir, index);
	std::cout << imgPath << std::endl;
	cv::Mat temp = cv::imread(imgPath);
	return temp;
}

// dir will need to be passed via env? Doesn't make sense to compile it in, and doesn't *really* make sense to use cwd
bool CimbWriter::load_tiles(std::string tile_dir)
{
	for (unsigned i = 0; i < _numSymbols; ++i)
		_tiles.push_back(load_tile(tile_dir, i));
	return true;
}

const cv::Mat& CimbWriter::encode(unsigned bits) const
{
	unsigned symbol = bits % _numSymbols;
	unsigned color = bits / _numSymbols;
	std::cout << fmt::format("symbol {}, color {}", symbol, color) << std::endl;
	return _tiles[symbol];
}
