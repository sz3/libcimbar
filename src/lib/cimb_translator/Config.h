/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "GridConf.h"

namespace cimbar
{
	class Config
	{
	protected:
		static cimbar::conf& active_conf()
		{
			static thread_local cimbar::conf cc = cimbar::Conf8x8();
			return cc;
		}

	public:
		static void update(int mode_val=0)
		{
			switch (mode_val)
			{
				case 4:
					active_conf() = cimbar::Conf8x8();
					active_conf().color_bits = 2;
					active_conf().legacy_mode = true;
					break;
				case 8:
					active_conf() = cimbar::Conf8x8();
					active_conf().color_bits = 3;
					active_conf().legacy_mode = true;
					break;
				case 68:
				default:
					active_conf() = cimbar::Conf8x8();
			}
		}

		static bool dark()
		{
			return true;
		}


		static bool legacy_mode()
		{
			return active_conf().legacy_mode;
		}

		static unsigned color_mode()
		{
			return active_conf().legacy_mode? 0 : 1; // unless we override per-thread?
		}

		static unsigned color_bits()
		{
			return active_conf().color_bits;
		}

		static unsigned symbol_bits()
		{
			return active_conf().symbol_bits;
		}

		static unsigned bits_per_cell()
		{
			return color_bits() + symbol_bits();
		}

		static unsigned ecc_bytes()
		{
			return active_conf().ecc_bytes;
		}

		static unsigned ecc_block_size()
		{
			return active_conf().ecc_block_size;
		}

		static unsigned image_size_x()
		{
			return active_conf().image_size_x;
		}

		static unsigned image_size_y()
		{
			return active_conf().image_size_y;
		}

		static unsigned anchor_size()
		{
			return 30;
		}

		static constexpr unsigned cell_size()
		{
			return 8;
		}

		static unsigned cell_spacing_x()
		{
			return active_conf().cell_spacing_x;
		}

		static unsigned cell_spacing_y()
		{
			return active_conf().cell_spacing_y;
		}

		static unsigned corner_padding_x();

		static unsigned corner_padding_y();

		static unsigned cell_offset()
		{
			return active_conf().cell_offset;
		}

		static unsigned cells_per_col_x()
		{
			return active_conf().cells_per_col_x;
		}

		static unsigned cells_per_col_y()
		{
			return active_conf().cells_per_col_y;
		}

		static unsigned total_cells();

		static unsigned decode_window_bits();

		// leave bitspercell here
		static unsigned capacity(unsigned bitspercell=0);

		static unsigned interleave_blocks()
		{
			return ecc_block_size();
		}

		static unsigned interleave_partitions()
		{
			return 2;
		}

		// leave bitspercell here
		static unsigned fountain_chunks_per_frame(unsigned bitspercell)
		{
			if (legacy_mode() or !active_conf().fountain_chunks_per_frame)
				return 10;
			return bitspercell * active_conf().fountain_chunks_per_frame;
		}

		static unsigned fountain_chunk_size();

		static unsigned compression_level()
		{
			return 16;
		}
	};
}

