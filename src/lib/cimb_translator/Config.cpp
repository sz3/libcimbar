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
	return 15;
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

}
