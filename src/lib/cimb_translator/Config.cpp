/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Config.h"
#include <cmath>

namespace cimbar {

unsigned Config::total_cells()
{
	return cells_per_col_x()*cells_per_col_y() - (corner_padding_x()*corner_padding_y() * 4);
}

unsigned Config::decode_window_bits()
{
	return std::pow(cell_size() + 2, 2);
}

unsigned Config::capacity(unsigned bitspercell)
{
	if (!bitspercell)
		bitspercell = bits_per_cell();
	return total_cells() * bitspercell / 8;
}

unsigned Config::corner_padding_x()
{
	return lrint(54.0 / cell_spacing_x());
}

unsigned Config::corner_padding_y()
{
	return lrint(54.0 / cell_spacing_y());
}

unsigned Config::fountain_chunk_size(unsigned ecc, unsigned bitspercell)
{
	// TODO: sanity checks?
	// this should neatly split into fountain_chunks_per_frame() [ex: 10] chunks per frame.
	// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
	const unsigned eccBlockSize = ecc_block_size();
	return capacity(bitspercell) * (eccBlockSize-ecc) / eccBlockSize / fountain_chunks_per_frame();
}

}
