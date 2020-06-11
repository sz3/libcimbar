#pragma once

#include "Point.h"
#include <vector>

class Midpoints
{
public:
	Midpoints(const std::vector<point<int>>& points)
		: _points(points)
	{
	}

	bool operator!() const
	{
		return _points.size() < 4;
	}

	const point<int>& top() const
	{
		return _points[0];
	}

	const point<int>& bottom() const
	{
		return _points[1];
	}

	const point<int>& left() const
	{
		return _points[2];
	}

	const point<int>& right() const
	{
		return _points[3];
	}

protected:
	std::vector<point<int>> _points;
};
