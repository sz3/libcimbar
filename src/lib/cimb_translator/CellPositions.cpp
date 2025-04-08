/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "CellPositions.h"
#include "Interleave.h"

CellPositions::positions_list CellPositions::compute_linear(cimbar::vec_xy spacing, cimbar::vec_xy dimensions, int offset, cimbar::vec_xy marker_size)
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
	int marker_offset_x = spacing.x * marker_size.x;
	int top_width = dimensions.x - marker_size.x - marker_size.x;
	int top_cells = top_width * marker_size.y;
	for (int i = 0; i < top_cells; ++i)
	{
		int x = (i % top_width) * spacing.x + marker_offset_x + offset;
		int y = (i / top_width) * spacing.y + offset_y;
		res.push_back({x, y});
	}

	int mid_y = marker_size.y * spacing.y;
	int mid_width = dimensions.x;
	int mid_height = dimensions.y - marker_size.y - marker_size.y;
	int mid_cells = mid_width * mid_height;
	for (int i = 0; i < mid_cells; ++i)
	{
		int x = (i % mid_width) * spacing.x + offset;
		int y = (i / mid_width) * spacing.y + mid_y + offset_y;
		res.push_back({x, y});
	}

	int bottom_y = (dimensions.y - marker_size.y) * spacing.y;
	int bottom_width = top_width;
	int bottom_cells = bottom_width * marker_size.y;
	for (int i = 0; i < bottom_cells; ++i)
	{
		int x = (i % bottom_width) * spacing.x + marker_offset_x + offset;
		int y = (i / bottom_width) * spacing.y + bottom_y + offset_y;
		res.push_back({x, y});
	}
	return res;
}

CellPositions::positions_list CellPositions::compute(cimbar::vec_xy spacing, cimbar::vec_xy dimensions, int offset, cimbar::vec_xy marker_size, int interleave_blocks, int interleave_partitions)
{
	CellPositions::positions_list pos = compute_linear(spacing, dimensions, offset, marker_size);
	if (interleave_blocks)
		return Interleave::interleave(pos, interleave_blocks, interleave_partitions);
	return pos;
}

CellPositions::CellPositions(cimbar::vec_xy spacing, cimbar::vec_xy dimensions, int offset, cimbar::vec_xy marker_size, int interleave_blocks, int interleave_partitions)
	: _positions(compute(spacing, dimensions, offset, marker_size, interleave_blocks, interleave_partitions))
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
