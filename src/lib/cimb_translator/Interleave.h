/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <vector>

namespace Interleave
{
	inline std::vector<unsigned> interleave_indices(unsigned size, unsigned num_chunks, unsigned partitions)
	{
		std::vector<unsigned> indices;
		if (num_chunks == 0)
		{
			for (unsigned i = 0; i < size; ++i)
				indices.push_back(i);
			return indices;
		}

		unsigned partitionSize = size/partitions;
		for (unsigned part = 0; part < size; part+=partitionSize)
			for (unsigned chunk = 0; chunk < num_chunks; ++chunk)
				for (unsigned i = chunk; i < partitionSize; i+=num_chunks)
					indices.push_back(i + part);
		return indices;
	}

	inline std::vector<unsigned> interleave_reverse(unsigned size, unsigned num_chunks, unsigned partitions)
	{
		std::vector<unsigned> indices = interleave_indices(size, num_chunks, partitions);
		std::vector<unsigned> inverted(indices.size(), 0);
		for (unsigned src = 0; src < indices.size(); ++src)
		{
			unsigned dst = indices[src];
			inverted[dst] = src;
		}
		return inverted;
	}

	template <typename PT>
	inline std::vector<PT> interleave(const std::vector<PT>& positions, unsigned num_chunks, unsigned partitions)
	{
		std::vector<PT> res;

		std::vector<unsigned> indices = interleave_indices(positions.size(), num_chunks, partitions);
		for (unsigned i = 0; i < indices.size(); ++i)
		{
			unsigned interleaveIdx = indices[i];
			res.push_back(positions[interleaveIdx]);
		}
		return res;
	}
}
