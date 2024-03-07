/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

namespace cimbar
{
	struct Conf5x5
	{
		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 2;
		static constexpr unsigned ecc_bytes = 40;
		static constexpr unsigned ecc_block_size = 216;
		static constexpr int image_size = 988;

		static constexpr unsigned cell_size = 5;
		static constexpr unsigned cell_offset = 9;
		static constexpr unsigned cells_per_col = 162;

		// unsigned interleave_partitions = 2;
		// int fountain_chunks_per_frame = -3;
	};

	struct Conf8x8_B
	{
		// unsigned id = 11; // 0xB

		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 4;
		static constexpr unsigned ecc_bytes = 30;
		static constexpr unsigned ecc_block_size = 155;
		static constexpr int image_size = 1024;

		static constexpr unsigned cell_size = 8;
		static constexpr unsigned cell_offset = 8;
		static constexpr unsigned cells_per_col = 112;

		// unsigned color_mode = 1;
		// unsigned interleave_partitions = 2;
		// int fountain_chunks_per_frame = -2;
	};


	struct Conf8x8_4C
	{
		// unsigned id = 4;

		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 4;
		static constexpr unsigned ecc_bytes = 30;
		static constexpr unsigned ecc_block_size = 155;
		static constexpr int image_size = 1024;

		static constexpr unsigned cell_size = 8;
		static constexpr unsigned cell_offset = 8;
		static constexpr unsigned cells_per_col = 112;

		// unsigned color_mode = 0;
		// unsigned interleave_partitions = 2;
		// int fountain_chunks_per_frame = 10;
	};
}
