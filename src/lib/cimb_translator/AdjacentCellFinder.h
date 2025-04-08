/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "CellPositions.h"
#include "util/vec_xy.h"
#include <array>

class AdjacentCellFinder
{
public:
	AdjacentCellFinder(const CellPositions::positions_list& positions, cimbar::vec_xy dimensions, cimbar::vec_xy marker_size);

	std::array<int, 4> find(int index) const;

	int dimensions_x() const;
	int marker_size_x() const;
	int calc_mid_width() const;

	int first_mid() const;

	int right(int index) const;
	int left(int index) const;
	int bottom(int index) const;
	int top(int index) const;

protected:
	int in_row_with_margin(int index) const;

protected:
	const CellPositions::positions_list& _positions;
	int _dimensionsX;
	int _markerSizeX;
	int _firstMid;
	int _firstBottom;
};
