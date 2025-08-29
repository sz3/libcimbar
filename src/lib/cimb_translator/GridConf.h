/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

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

		int fountain_chunks_per_frame = 2;
		bool legacy_mode = false;
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

			fountain_chunks_per_frame = 3;
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

			fountain_chunks_per_frame = 4;
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

			fountain_chunks_per_frame = 2;
		}
	};

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

			fountain_chunks_per_frame = 2;
		}
	};
}
