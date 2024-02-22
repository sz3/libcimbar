/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Config.h"
#include <cmath>

namespace cimbar {

unsigned Config::total_cells()
{
	return std::pow(cells_per_col(), 2) - std::pow(corner_padding(), 2) * 4;
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

unsigned Config::corner_padding()
{
	return lrint(54.0 / cell_spacing());
}

unsigned Config::fountain_chunk_size(unsigned ecc, unsigned bitspercell, bool legacy_mode)
{
	// TODO: sanity checks?
	// this should neatly split into fountain_chunks_per_frame() [ex: 10] chunks per frame.
	// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
	const unsigned eccBlockSize = ecc_block_size();
	return capacity(bitspercell) * (eccBlockSize-ecc) / eccBlockSize / fountain_chunks_per_frame(bitspercell, legacy_mode);
}

}
