#pragma once

#include "Corners.h"
#include "Midpoints.h"
#include "Point.h"
#include <optional>
#include <vector>
#include <utility>

namespace Geometry
{

	std::optional<point> line_intersection(const std::pair<point, point>& lineA, const std::pair<point, point>& lineB)
	{
		auto compute = [] (const point& p, const point& q) {
			int xdiff = q.x() - p.x();
			int ydiff = p.y() - q.y();
			int determinant = q.x() * p.y() - p.x() * q.y();
			return std::tuple<int, int, int>(xdiff, ydiff, determinant);
		};

		auto [ax, ay, adet] = compute(lineA.first, lineA.second);
		auto [bx, by, bdet] = compute(lineB.first, lineB.second);

		int D = ay * bx - ax * by;
		if (!D)
			return std::nullopt;

		int Dx = adet * bx - ax * bdet;
		int Dy = ay * bdet - adet * by;
		return point({Dx / D, Dy / D});
	}

	Midpoints calculate_midpoints(const Corners& sq)
	{
		std::vector<point> mids;
		using line = std::pair<point, point>;

		line cross1 = {sq.top_left(), sq.bottom_right()};
		line cross2 = {sq.top_right(), sq.bottom_left()};
		auto center = line_intersection(cross1, cross2);
		if (!center)
			return mids;

		line right = {sq.top_right(), sq.bottom_right()};
		line left = {sq.top_left(), sq.bottom_left()};
		auto leftRightInf = line_intersection(right, left);
		if (!leftRightInf)
			return mids;
		line vertical = {*center, *leftRightInf};

		line top = {sq.top_left(), sq.top_right()};
		line bottom = {sq.bottom_left(), sq.bottom_right()};
		auto topBottomInf = line_intersection(top, bottom);
		if (!topBottomInf)
			return mids;
		line horizontal = {*center, *topBottomInf};

		// there are some corner cases that need to be handled here...
		auto tmid = line_intersection(top, vertical);
		if (!tmid)
			mids.push_back(point::NONE());
		else
			mids.push_back(*tmid);

		auto bmid = line_intersection(bottom, vertical);
		if (!bmid)
			mids.push_back(point::NONE());
		else
			mids.push_back(*bmid);

		auto lmid = line_intersection(left, horizontal);
		if (!lmid)
			mids.push_back(point::NONE());
		else
			mids.push_back(*lmid);

		auto rmid = line_intersection(right, horizontal);
		if (!rmid)
			mids.push_back(point::NONE());
		else
			mids.push_back(*rmid);

		return mids;
	}
}
