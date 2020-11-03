/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Config.h"

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
	return 4;
}

unsigned Config::bits_per_cell()
{
	return color_bits() + symbol_bits();
}

unsigned Config::ecc_bytes()
{
	return 30;
}

int Config::image_size()
{
	return 1024;
}

unsigned Config::anchor_size()
{
	return 30;
}

unsigned Config::cell_size()
{
	return 8;
}

unsigned Config::cell_spacing()
{
	return cell_size() + 1;
}

unsigned Config::num_cells()
{
	return 112;
}

unsigned Config::corner_padding()
{
	return 6;
}

unsigned Config::interleave_blocks()
{
	return 155;
}

unsigned Config::interleave_partitions()
{
	return 2;
}

unsigned Config::fountain_chunk_size(unsigned ecc, unsigned bitspercell)
{
	// this calculation is based off the 112x112-6 grid.
	// in that grid, we have 155 * bits_per_cell * 10 total bytes of data.
	// so this neatly splits into 10 chunks per frame.
	// ex: 690=6900/10 for ecc=40.
	if (!bitspercell)
		bitspercell = bits_per_cell();

	// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
	return (155-ecc) * bitspercell * 10 / fountain_chunks_per_frame();
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
