/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "AdjacentCellFinder.h"

AdjacentCellFinder::AdjacentCellFinder(const CellPositions::positions_list& positions, int dimensions, int marker_size)
	: _positions(positions)
	, _dimensions(dimensions)
	, _markerSize(marker_size)
{
	int midDimensions = dimensions - marker_size - marker_size;
	int midCells = dimensions * midDimensions;
	int edgeCells = midDimensions * marker_size;
	_firstMid = edgeCells;
	_firstBottom = edgeCells + midCells;
}

std::array<int, 4> AdjacentCellFinder::find(int index) const
{
	std::array<int,4> adj = {
		right(index), left(index), bottom(index), top(index)
	};
	return adj;
}

int AdjacentCellFinder::dimensions() const
{
	return _dimensions;
}

int AdjacentCellFinder::marker_size() const
{
	return _markerSize;
}

int AdjacentCellFinder::in_row_with_margin(int index) const
{
	if (index < _firstMid)  // top
		return 1;
	if (index < _firstBottom)  // middle
		return 0;
	else
		return 1; // bottom
}

int AdjacentCellFinder::right(int index) const
{
	if (index < 0 || index >= (int)_positions.size() - 1)
		return -1;
	int next = index + 1;
	if (_positions[next].first < _positions[index].first)
		return -1;
	return next;
}

int AdjacentCellFinder::left(int index) const
{
	int next = index-1;
	if (next < 0)
		return -1;
	if (_positions[next].first > _positions[index].first)  // loop
		return -1;
	return next;
}

int AdjacentCellFinder::bottom(int index) const
{
	if (index < 0 || index >= (int)_positions.size())
		return -1;
	int increment = _dimensions;
	if (in_row_with_margin(index))
		increment -= _markerSize;
	int next = index + increment;
	if (in_row_with_margin(next))
		next -= _markerSize;
	if (next < 0 || next >= (int)_positions.size())
		return -1;
	if (_positions[next].first != _positions[index].first)
		return -1;
	return next;
}

int AdjacentCellFinder::top(int index) const
{
	int increment = _dimensions;
	if (in_row_with_margin(index))
		increment -= _markerSize;
	int next = index - increment;
	if (in_row_with_margin(next))
		next += _markerSize;

	if (next < 0)
		return -1;
	if (_positions[next].first != _positions[index].first)  // near anchor
		return -1;
	return next;
}
