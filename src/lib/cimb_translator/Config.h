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
		static cimbar::conf temp_conf(int mode_val=0)
		{
			cimbar::conf cc;
			switch (mode_val)
			{
				case 4:
					cc = cimbar::Conf8x8();
					cc.color_bits = 2;
					cc.legacy_mode = true;
					return cc;
				case 8:
					cc = cimbar::Conf8x8();
					cc.color_bits = 3;
					cc.legacy_mode = true;
					return cc;
				case 67:
					return cimbar::Conf8x8_mini();
				case 68:
				default:
					return cimbar::Conf8x8();
			}
		}

		static void update(int mode_val=0)
		{
			active_conf() = temp_conf(mode_val);
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
			return active_conf().bits_per_cell();
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

		static unsigned corner_padding_x()
		{
			return active_conf().corner_padding_x();
		}

		static unsigned corner_padding_y()
		{
			return active_conf().corner_padding_y();
		}

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

		static unsigned total_cells()
		{
			return active_conf().total_cells();
		}

		// leave bitspercell here
		static unsigned capacity(unsigned bitspercell=0)
		{
			return active_conf().capacity(bitspercell);
		}

		static unsigned interleave_blocks()
		{
			return ecc_block_size();
		}

		static unsigned interleave_partitions()
		{
			return 2;
		}

		static unsigned fountain_chunks_per_frame(unsigned bitspercell=0)
		{
			return active_conf().fountain_chunks_per_frame(bitspercell);
		}

		static unsigned fountain_chunk_size(unsigned bitspercell=0)
		{
			return active_conf().fountain_chunk_size(bitspercell);
		}

		static unsigned compression_level()
		{
			return 16;
		}
	};
}

