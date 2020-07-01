#include "CellPosition.h"
#include "Interleave.h"

#include <vector>
#include <utility>

CellPosition::positions_list CellPosition::compute_linear(int spacing, int dimensions, int offset, int marker_size)
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
	int marker_offset_x = spacing * marker_size;
	int top_width = dimensions - marker_size - marker_size;
	int top_cells = top_width * marker_size;
	for (int i = 0; i < top_cells; ++i)
	{
		int x = (i % top_width) * spacing + marker_offset_x + offset;
		int y = (i / top_width) * spacing + offset_y;
		res.push_back({x, y});
	}

	int mid_y = marker_size * spacing;
	int mid_width = dimensions;
	int mid_cells = mid_width * top_width;  // top_width is also "mid_height"
	for (int i = 0; i < mid_cells; ++i)
	{
		int x = (i % mid_width) * spacing + offset;
		int y = (i / mid_width) * spacing + mid_y + offset_y;
		res.push_back({x, y});
	}

	int bottom_y = (dimensions - marker_size) * spacing;
	int bottom_width = top_width;
	int bottom_cells = bottom_width * marker_size;
	for (int i = 0; i < bottom_cells; ++i)
	{
		int x = (i % bottom_width) * spacing + marker_offset_x + offset;
		int y = (i / bottom_width) * spacing + bottom_y + offset_y;
		res.push_back({x, y});
	}
	return res;
}

CellPosition::positions_list CellPosition::compute(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks)
{
	CellPosition::positions_list pos = compute_linear(spacing, dimensions, offset, marker_size);
	if (interleave_blocks)
		return Interleave::interleave(pos, interleave_blocks);
	return pos;
}

CellPosition::CellPosition(int spacing, int dimensions, int offset, int marker_size, int interleave_blocks)
    : _positions(compute(spacing, dimensions, offset, marker_size, interleave_blocks))
{
	reset();
}

unsigned CellPosition::index() const
{
	return _index;
}

size_t CellPosition::count() const
{
	return _positions.size();
}

void CellPosition::reset()
{
	_index = 0;
}

bool CellPosition::done() const
{
	return _index >= _positions.size();
}

const CellPosition::coordinate& CellPosition::next()
{
	return _positions[_index++];
}

