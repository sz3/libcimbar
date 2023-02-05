/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Config.h"
#include <cmath>

namespace cimbar {

bool Config::dark()
{
	return true;
}

unsigned Config::color_bits()
{
	return 2;
}

unsigned Config::symbol_bits()
{
	return 2;
}

unsigned Config::bits_per_cell()
{
	return color_bits() + symbol_bits();
}

unsigned Config::ecc_bytes()
{
	return 40;
}

unsigned Config::ecc_block_size()
{
	return 216;
}

unsigned Config::cell_size()
{
	return 5;
}

unsigned Config::cell_spacing()
{
	return cell_size() + 1;
}

unsigned Config::cell_offset()
{
	return 9;
}

unsigned Config::cells_per_col()
{
	return 162;
}

unsigned Config::total_cells()
{
	return std::pow(cells_per_col(), 2) - std::pow(corner_padding(), 2) * 4;
}

unsigned Config::capacity(unsigned bitspercell)
{
	if (!bitspercell)
		bitspercell = bits_per_cell();
	return total_cells() * bitspercell / 8;
}

unsigned Config::corner_padding()
{
	return lrint(54.0 / cell_spacing());
}

unsigned Config::interleave_blocks()
{
	return ecc_block_size();
}

unsigned Config::interleave_partitions()
{
	return 2;
}

unsigned Config::fountain_chunk_size(unsigned ecc, unsigned bitspercell)
{
	// TODO: sanity checks?
	// this should neatly split into fountain_chunks_per_frame() [ex: 10] chunks per frame.
	// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
	const unsigned eccBlockSize = ecc_block_size();
	return capacity(bitspercell) * (eccBlockSize-ecc) / eccBlockSize / fountain_chunks_per_frame();
}

unsigned Config::fountain_chunks_per_frame()
{
	return 10;
}

unsigned Config::compression_level()
{
	return 6;
}

}
