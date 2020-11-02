/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

// probably defaults with a pimpl to read/cache if there's a (yaml?) config locally?
namespace cimbar
{
	namespace Config
	{
		bool dark();

		unsigned color_bits();
		unsigned symbol_bits();
		unsigned bits_per_cell();
		unsigned ecc_bytes();

		int image_size();
		unsigned anchor_size();

		unsigned cell_size();
		unsigned cell_spacing();
		unsigned num_cells();
		unsigned corner_padding();
		unsigned interleave_blocks();
		unsigned interleave_partitions();

		unsigned fountain_chunk_size(unsigned ecc, unsigned bitspercell=0);
		unsigned fountain_chunks_per_frame();
		unsigned compression_level();
	}
}
