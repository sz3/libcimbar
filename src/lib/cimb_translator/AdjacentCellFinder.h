#pragma once

#include "CellPositions.h"
#include <array>

class AdjacentCellFinder
{
public:
	AdjacentCellFinder(const CellPositions::positions_list& positions, int dimensions, int marker_size)
		: _positions(positions)
		, _markerSize(marker_size)
		, _dimensions(dimensions)
	{
		int midDimensions = dimensions - marker_size - marker_size;
		int midCells = dimensions * midDimensions;
		int edgeCells = midDimensions * marker_size;
		_firstMid = edgeCells;
		_firstBottom = edgeCells + midCells;
	}

	std::array<int, 4> find(int index)
	{
		std::array<int,4> adj = {
			right(index), left(index), bottom(index), top(index)
		};
		return adj;
	}

protected:
	int in_row_with_margin(int index) const
	{
		if (index < _firstMid)  // top
			return 1;
		if (index < _firstBottom)  // middle
			return 0;
		else
			return 1; // bottom
	}

	int right(int index) const
	{
		int next = index+1;
		if (next >= _positions.size())
			return -1;
		if (_positions[next].first < _positions[index].first)  // loop
			return -1;
		return next;
	}

	int left(int index) const
	{
		int next = index-1;
		if (next < 0)
			return -1;
		if (_positions[next].first > _positions[index].first)  // loop
			return -1;
		return next;
	}

	int bottom(int index) const
	{
		int increment = _dimensions;
		if (in_row_with_margin(index))
			increment -= _markerSize;
		int next = index + increment;
		if (in_row_with_margin(next))
			next -= _markerSize;

		if (next >= _positions.size())
			return -1;
		return next;
	}

	int top(int index) const
	{
		int increment = _dimensions;
		if (in_row_with_margin(index))
			increment -= _markerSize;
		int next = index - increment;
		if (in_row_with_margin(next))
			next += _markerSize;

		if (next < 0)
			return -1;
		return next;
	}

protected:
	const CellPositions::positions_list& _positions;
	int _dimensions;
	int _markerSize;
	int _firstMid;
	int _firstBottom;
};
