/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <cstddef>
#include <utility>
#include <vector>

class CellPositions
{
public:
	using coordinate = std::pair<int, int>;
	using positions_list = std::vector<coordinate>;

	static positions_list compute_linear(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y);
	static positions_list compute(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y, int interleave_blocks=0, int interleave_partitions=1);

public:
	CellPositions(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y, int interleave_blocks=0, int interleave_partitions=1);

	unsigned index() const;
	size_t count() const;
	void reset();
	bool done() const;
	const coordinate& next();

	const positions_list& positions() const;

protected:
	unsigned _index;
	positions_list _positions;
};
