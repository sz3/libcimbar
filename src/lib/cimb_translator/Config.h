/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "GridConf.h"

namespace cimbar
{
	class Config
	{
	protected:
		using GridConf = Conf8x8;

	public:
		static constexpr bool dark()
		{
			return true;
		}

		static constexpr unsigned color_mode()
		{
			return 1; // unless we override per-thread?
		}

		static constexpr unsigned color_bits()
		{
			return GridConf::color_bits;
		}

		static constexpr unsigned symbol_bits()
		{
			return GridConf::symbol_bits;
		}

		static constexpr unsigned bits_per_cell()
		{
			return color_bits() + symbol_bits();
		}

		static constexpr unsigned ecc_bytes()
		{
			return GridConf::ecc_bytes;
		}

		static constexpr unsigned ecc_block_size()
		{
			return GridConf::ecc_block_size;
		}

		static constexpr int image_size()
		{
			return GridConf::image_size;
		}

		static constexpr unsigned anchor_size()
		{
			return 30;
		}

		static constexpr unsigned cell_size()
		{
			return GridConf::cell_size;
		}

		static constexpr unsigned cell_spacing()
		{
			return cell_size() + 1;
		}

		static unsigned corner_padding();

		static constexpr unsigned cell_offset()
		{
			return GridConf::cell_offset;
		}

		static constexpr unsigned cells_per_col()
		{
			return GridConf::cells_per_col;
		}

		static unsigned total_cells();

		static unsigned decode_window_bits();

		static unsigned capacity(unsigned bitspercell=0);

		static constexpr unsigned interleave_blocks()
		{
			return ecc_block_size();
		}

		static constexpr unsigned interleave_partitions()
		{
			return 2;
		}

		static constexpr unsigned fountain_chunks_per_frame(unsigned bitspercell, bool legacy_mode)
		{
			return legacy_mode? 10 : bitspercell << 1;
		}

		static unsigned fountain_chunk_size(unsigned ecc, unsigned bitspercell, bool legacy_mode);

		static constexpr unsigned compression_level()
		{
			return 16;
		}
	};
}

