#pragma once

#include <vector>

namespace Interleave
{
	inline unsigned block_size(unsigned size, int num_chunks)
	{
		return size / num_chunks;
	}

	inline std::vector<unsigned> interleave_indices(unsigned size, int num_chunks)
	{
		std::vector<unsigned> indices;
		if (num_chunks == 0)
		{
			for (int i = 0; i < size; ++i)
				indices.push_back(i);
			return indices;
		}

		int blockSize = block_size(size, num_chunks);
		for (int chunk = 0; chunk < num_chunks; ++chunk)
			for (int offset = 0; offset < blockSize; ++offset)
			{
				unsigned i = (offset * num_chunks) + chunk;
				indices.push_back(i);
			}
		return indices;
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
