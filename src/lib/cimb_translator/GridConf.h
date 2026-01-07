/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cmath>

namespace cimbar
{
	struct conf
	{
		unsigned color_bits = 0;
		unsigned symbol_bits = 0;
		unsigned ecc_bytes = 0;
		unsigned ecc_block_size = 0;
		unsigned image_size_x = 1;
		unsigned image_size_y = image_size_x;

		unsigned cell_size = 8;
		unsigned cell_spacing_x = cell_size+1;
		unsigned cell_spacing_y = cell_size+1;
		unsigned cell_offset = 9;
		unsigned cells_per_col_x = 0;
		unsigned cells_per_col_y = cells_per_col_x;

		int fountain_chunks_scalar = 2;
		bool legacy_mode = false;

		unsigned bits_per_cell() const
		{
			return color_bits + symbol_bits;
		}

		unsigned corner_padding_x() const
		{
			return lrint(54.0 / cell_spacing_x);
		}

		unsigned corner_padding_y() const
		{
			return lrint(54.0 / cell_spacing_y);
		}

		unsigned total_cells() const
		{
			return cells_per_col_x*cells_per_col_y - (corner_padding_x()*corner_padding_y() * 4);
		}

		unsigned capacity(unsigned bitspercell) const
		{
			if (!bitspercell)
				bitspercell = bits_per_cell();
			return total_cells() * bitspercell / 8;
		}

		unsigned fountain_chunks_per_frame(unsigned bitspercell=0) const
		{
			if (!bitspercell)
				bitspercell = bits_per_cell();
			if (fountain_chunks_scalar < 0)
				return -fountain_chunks_scalar;
			return bitspercell * fountain_chunks_scalar;
		}

		unsigned fountain_chunk_size(unsigned bitspercell=0) const
		{
			// TODO: sanity checks?
			// this should neatly split into fountain_chunks_per_frame() [ex: 10] chunks per frame.
			// the other reasonable settings for fountain_chunks_per_frame are `2` and `5`
			if (!bitspercell)
				bitspercell = bits_per_cell();
			return capacity(bitspercell) * (ecc_block_size-ecc_bytes) / ecc_block_size / fountain_chunks_per_frame(bitspercell);
		}

	};

	struct Conf5x5 : conf
	{
		Conf5x5()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 2;
			ecc_bytes = 40;
			ecc_block_size = 216;
			image_size_x = 988;
			image_size_y = image_size_x;

			cell_size = 5;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 162;
			cells_per_col_y = cells_per_col_x;

			fountain_chunks_scalar = 3;
		}
	};

	struct Conf5x5d : conf
	{
		Conf5x5d()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 2;
			ecc_bytes = 35;
			ecc_block_size = 182;
			image_size_x = 958;
			image_size_y = image_size_x;

			cell_size = 5;
			cell_spacing_x = cell_size;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 188;
			cells_per_col_y = 157;

			fountain_chunks_scalar = 4;
		}
	};



	struct Conf8x8 : conf
	{
		Conf8x8()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 30;
			ecc_block_size = 155;
			image_size_x = 1024;
			image_size_y = image_size_x;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 8;
			cells_per_col_x = 112;
			cells_per_col_y = cells_per_col_x;

			fountain_chunks_scalar = 2;
		}
	};

	/*
	 *
	 * 	struct Conf8x8_mini : conf
	{
		Conf8x8_mini()
			: conf()
		{
			//96,91 = 8592, 6444 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 36;
			ecc_block_size = 179;
			image_size_x = 880;
			image_size_y = 835;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 96;
			cells_per_col_y = 91;

			fountain_chunks_scalar = 2;
		}
	};

	*/
	/*
	 * 	struct Conf8x8_mini : conf
	{
		Conf8x8_mini()
			: conf()
		{
			// 93,80 = 7296, 5472 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 46;
			ecc_block_size = 228;
			image_size_x = 853;
			image_size_y = 736;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 93;
			cells_per_col_y = 80;

			fountain_chunks_scalar = 1;
		}
	};*/

	struct Conf8x8_mini : conf
	{
		Conf8x8_mini()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 36;
			ecc_block_size = 179;
			image_size_x = 1024;
			image_size_y = 720;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 112;
			cells_per_col_y = 78;

			fountain_chunks_scalar = 2;
		}
	};

	struct Conf8x8_micro : conf
	{
		Conf8x8_micro()
			: conf()
		{
			// 80,69 = 5376, 4032 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 33;
			ecc_block_size = 168;
			image_size_x = 736;
			image_size_y = 637;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 80;
			cells_per_col_y = 69;

			fountain_chunks_scalar = 1;
		}
	};

	struct Conf8x8_wide : conf
	{
		Conf8x8_wide()
			: conf()
		{
			// 147,112 = 16320, 12240 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 34;//51;
			ecc_block_size = 170;//255;
			image_size_x = 1339;
			image_size_y = 1024;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 147;
			cells_per_col_y = 112;

			fountain_chunks_scalar = 3;
		}
	};

	/*struct Conf8x8_wide3 : conf
	{
		Conf8x8_wide()
			: conf()
		{
			// 135,112 = 14976, 11232 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 31;
			ecc_block_size = 156;
			image_size_x = 1231;
			image_size_y = 1024;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 135;
			cells_per_col_y = 112;

			fountain_chunks_scalar = 3;
		}
	};*/

	/*struct Conf8x8_wide4 : conf
	{
		Conf8x8_wide()
			: conf()
		{
			// 127,112 = 14080, 10560 bytes
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 35;
			ecc_block_size = 176;
			image_size_x = 1159;
			image_size_y = 1024;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 127;
			cells_per_col_y = 112;

			fountain_chunks_scalar = 3;
		}
	};*/


	/*struct Conf8x8_wide : conf
	{
		Conf8x8_wide()
			: conf()
		{
			color_bits = 2;
			symbol_bits = 4;
			ecc_bytes = 50;
			ecc_block_size = 250;
			image_size_x = 1474;
			image_size_y = 1024;

			cell_size = 8;
			cell_spacing_x = cell_size+1;
			cell_spacing_y = cell_size+1;
			cell_offset = 9;
			cells_per_col_x = 162;
			cells_per_col_y = 112;

			fountain_chunks_scalar = 3; // or -20
		}
	};*/
}
