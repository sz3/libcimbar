/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "GridConf.h"
#include <cmath>

// probably defaults with a pimpl to read/cache if there's a (yaml?) config locally?
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

		static constexpr unsigned corner_padding()
		{
			return lrint(54.0 / cell_spacing());
		}

		static constexpr unsigned cell_offset()
		{
			return GridConf::cell_offset;
		}

		static constexpr unsigned cells_per_col()
		{
			return GridConf::cells_per_col;
		}

		static constexpr unsigned total_cells()
		{
			return std::pow(cells_per_col(), 2) - std::pow(corner_padding(), 2) * 4;
		}

		static constexpr unsigned decode_window_bits()
		{
			return std::pow(cell_size() + 2, 2);
		}

		inline static unsigned capacity(unsigned bitspercell=0)
		{
			if (!bitspercell)
				bitspercell = bits_per_cell();
			return total_cells() * bitspercell / 8;
		}

		static constexpr unsigned interleave_blocks()
		{
			return ecc_block_size();
		}

		static constexpr unsigned interleave_partitions()
		{
			return 2;
		}

		static constexpr unsigned fountain_chunks_per_frame()
		{
			return 10;
		}

		inline static unsigned fountain_chunk_size(unsigned ecc, unsigned bitspercell=0)
		{
			// TODO: sanity checks?
			// this should neatly split into fountain_chunks_per_frame() [ex: 10] chunks per frame.
			// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
			const unsigned eccBlockSize = ecc_block_size();
			return capacity(bitspercell) * (eccBlockSize-ecc) / eccBlockSize / fountain_chunks_per_frame();
		}

		static constexpr unsigned compression_level()
		{
			return 6;
		}
	};
}
