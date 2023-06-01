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
	};

	struct Conf8x8
	{
		static constexpr unsigned color_bits = 2;
		static constexpr unsigned symbol_bits = 4;
		static constexpr unsigned ecc_bytes = 30;
		static constexpr unsigned ecc_block_size = 155;
		static constexpr int image_size = 1024;

		static constexpr unsigned cell_size = 8;
		static constexpr unsigned cell_offset = 8;
		static constexpr unsigned cells_per_col = 112;
	};
}
