/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CellPositions.h"
#include "Interleave.h"

CellPositions::positions_list CellPositions::compute_linear(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y)
{
	/*
	 * ex: if dimensions == 128, and marker_size == 8:
		8 tiles at top is 128-16 == 112
		8 tiles at bottom is also 128-16 == 112

		structure would be:
		112 * 8
		128 * 112
		112 * 8
	*/
	positions_list res;
	int offset_y = offset;
	int marker_offset_x = spacing_x * marker_size_x;
	int top_width = dimensions_x - marker_size_x - marker_size_x;
	int top_cells = top_width * marker_size_y;
	for (int i = 0; i < top_cells; ++i)
	{
		int x = (i % top_width) * spacing_x + marker_offset_x + offset;
		int y = (i / top_width) * spacing_y + offset_y;
		res.push_back({x, y});
	}

	int mid_y = marker_size_y * spacing_y;
	int mid_width = dimensions_x;
	int mid_height = dimensions_y - marker_size_y - marker_size_y;
	int mid_cells = mid_width * mid_height;
	for (int i = 0; i < mid_cells; ++i)
	{
		int x = (i % mid_width) * spacing_x + offset;
		int y = (i / mid_width) * spacing_y + mid_y + offset_y;
		res.push_back({x, y});
	}

	int bottom_y = (dimensions_y - marker_size_y) * spacing_y;
	int bottom_width = top_width;
	int bottom_cells = bottom_width * marker_size_y;
	for (int i = 0; i < bottom_cells; ++i)
	{
		int x = (i % bottom_width) * spacing_x + marker_offset_x + offset;
		int y = (i / bottom_width) * spacing_y + bottom_y + offset_y;
		res.push_back({x, y});
	}
	return res;
}

CellPositions::positions_list CellPositions::compute(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y, int interleave_blocks, int interleave_partitions)
{
	CellPositions::positions_list pos = compute_linear(spacing_x, spacing_y, dimensions_x, dimensions_y, offset, marker_size_x, marker_size_y);
	if (interleave_blocks)
		return Interleave::interleave(pos, interleave_blocks, interleave_partitions);
	return pos;
}

CellPositions::CellPositions(int spacing_x, int spacing_y, int dimensions_x, int dimensions_y, int offset, int marker_size_x, int marker_size_y, int interleave_blocks, int interleave_partitions)
	: _positions(compute(spacing_x, spacing_y, dimensions_x, dimensions_y, offset, marker_size_x, marker_size_y, interleave_blocks, interleave_partitions))
{
	reset();
}

unsigned CellPositions::index() const
{
	return _index;
}

size_t CellPositions::count() const
{
	return _positions.size();
}

void CellPositions::reset()
{
	_index = 0;
}

bool CellPositions::done() const
{
	return _index >= _positions.size();
}

const CellPositions::coordinate& CellPositions::next()
{
	return _positions[_index++];
}

const CellPositions::positions_list& CellPositions::positions() const
{
	return _positions;
}
