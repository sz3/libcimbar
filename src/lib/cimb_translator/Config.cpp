#include "Config.h"

bool cimbar::Config::dark()
{
	return true;
}

unsigned cimbar::Config::color_bits()
{
	return 2;
}

unsigned cimbar::Config::symbol_bits()
{
	return 4;
}

unsigned cimbar::Config::bits_per_cell()
{
	return color_bits() + symbol_bits();
}

unsigned cimbar::Config::image_size()
{
	return 1024;
}

unsigned cimbar::Config::anchor_size()
{
	return 30;
}

unsigned cimbar::Config::cell_size()
{
	return 8;
}

unsigned cimbar::Config::cell_spacing()
{
	return cell_size() + 1;
}

unsigned cimbar::Config::num_cells()
{
	return 112;
}

unsigned cimbar::Config::corner_padding()
{
	return 6;
}
