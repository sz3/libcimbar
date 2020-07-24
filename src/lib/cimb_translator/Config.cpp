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
	return 40;
}

unsigned Config::image_size()
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

unsigned Config::fountain_chunk_size(unsigned ecc)
{
	// this calculation is based off the 112x112-6 grid.
	// in that grid, we have 155 * bits_per_cell * 10 total bytes of data.
	// so this neatly splits into 10 chunks per frame.
	// ex: 690=6900/10 for ecc=40.

	// might double it to 5 per frame.
	return (155-ecc) * bits_per_cell() * 10 / fountain_chunks_per_frame();
}

unsigned Config::fountain_chunks_per_frame()
{
	return 10;
}

}
