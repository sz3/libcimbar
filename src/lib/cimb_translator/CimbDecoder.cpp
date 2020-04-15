#include "CimbDecoder.h"

#include "CimbCommon.h"
#include "image_hash/average_hash.h"

using std::string;

CimbDecoder::CimbDecoder(unsigned symbol_bits, unsigned color_bits)
    : _symbolBits(symbol_bits)
    , _numSymbols(1 << symbol_bits)
    , _numColors(1 << color_bits)
{
	load_tiles(CimbCommon::getTileDir(_symbolBits));
}

uint64_t CimbDecoder::get_tile_hash(string tile_dir, unsigned symbol)
{
	cv::Mat tile = CimbCommon::getTile(tile_dir, symbol);
	return image_hash::average_hash(tile);
}

bool CimbDecoder::load_tiles(std::string tile_dir)
{
	unsigned numTiles = _numSymbols;
	for (unsigned i = 0; i < numTiles; ++i)
		_tileHashes.push_back(get_tile_hash(tile_dir, i));
	return true;
}

unsigned CimbDecoder::get_best_symbol(uint64_t hash)
{
	unsigned min_distance = 1000;
	unsigned best_fit = 0;
	for (unsigned i = 0; i < _tileHashes.size(); ++i)
	{
		unsigned distance = hash xor _tileHashes[i];
		if (distance < min_distance)
		{
			min_distance = distance;
			best_fit = i;
			if (min_distance == 0)
				break;
		}
	}
	return best_fit;
}

unsigned CimbDecoder::decode_symbol(const cv::Mat& cell)
{
	uint64_t hash = image_hash::average_hash(cell);
	return get_best_symbol(hash);
}

unsigned CimbDecoder::decode_color(const cv::Mat& cell)
{
	if (_numColors <= 1)
		return 0;

}

unsigned CimbDecoder::decode(const cv::Mat& cell)
{
	unsigned bits = decode_symbol(cell);
	//bits |= decode_color(cell) << _symbolBits;
	return bits;
}
