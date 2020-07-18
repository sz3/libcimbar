#pragma once

// probably defaults with a pimpl to read/cache if there's a (yaml?) config locally?
namespace cimbar
{
	namespace Config
	{
		bool dark();
		bool gpu_opt();

		unsigned color_bits();
		unsigned symbol_bits();
		unsigned bits_per_cell();
		unsigned ecc_bytes();

		unsigned image_size();
		unsigned anchor_size();

		unsigned cell_size();
		unsigned cell_spacing();
		unsigned num_cells();
		unsigned corner_padding();
		unsigned interleave_blocks();
	}
}
