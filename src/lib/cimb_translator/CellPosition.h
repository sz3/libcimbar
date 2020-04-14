#pragma once

#include <vector>
#include <utility>

class CellPosition
{
public:
	using coordinate = std::pair<int, int>;
	using positions_list = std::vector<coordinate>;

	static positions_list compute(int spacing, int dimensions, int offset, int marker_size)
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
		int offset_y = offset + 1;
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

public:
	CellPosition(int spacing, int dimensions, int offset=0, int marker_size=6)
	    : _positions(compute(spacing, dimensions, offset, marker_size))
	{
		reset();
	}

	void reset()
	{
		_index = 0;
	}

	bool done() const
	{
		return _index >= _positions.size();
	}

	const coordinate& next()
	{
		return _positions[_index++];
	}

protected:
	unsigned _index;
	positions_list _positions;
};
