#pragma once

#include <vector>

namespace Interleave
{
	inline std::vector<unsigned> interleave_indices(unsigned size, int num_chunks)
	{
		std::vector<unsigned> indices;
		if (num_chunks == 0)
		{
			for (int i = 0; i < size; ++i)
				indices.push_back(i);
			return indices;
		}

		for (int chunk = 0; chunk < num_chunks; ++chunk)
			for (int i = chunk; i < size; i+=num_chunks)
				indices.push_back(i);
		return indices;
	}

	inline std::vector<unsigned> interleave_reverse(unsigned size, int num_chunks)
	{
		std::vector<unsigned> indices = interleave_indices(size, num_chunks);
		std::vector<unsigned> inverted(indices.size(), 0);
		for (unsigned src = 0; src < indices.size(); ++src)
		{
			unsigned dst = indices[src];
			inverted[dst] = src;
		}
		return inverted;
	}

	template <typename PT>
	inline std::vector<PT> interleave(const std::vector<PT>& positions, int num_chunks)
	{
		std::vector<PT> res;

		std::vector<unsigned> indices = interleave_indices(positions.size(), num_chunks);
		for (int i = 0; i < indices.size(); ++i)
		{
			unsigned interleaveIdx = indices[i];
			res.push_back(positions[interleaveIdx]);
		}
		return res;
	}
}
