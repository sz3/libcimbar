#include "Scanner.h"

#include "Corners.h"
#include "EdgeScanState.h"
#include "Geometry.h"
#include "ScanState.h"
#include "serialize/format.h"
#include <algorithm>

namespace {
	struct size_sort
	{
		inline bool operator() (const Anchor& an1, const Anchor& an2)
		{
			return an1.size() > an2.size();
		}
	};
}

int Scanner::anchor_size() const
{
	return _anchorSize;
}

bool Scanner::test_pixel(int x, int y) const
{
	uchar pixel = _img.at<uchar>(y, x);
	if (_dark)
		return pixel > 127;
	else
		return pixel < 127;
}

std::vector<Anchor> Scanner::deduplicate_candidates(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> merged;
	for (const Anchor& c : candidates)
	{
		bool foundMerge = false;
		for (Anchor& m : merged)
		{
			if (m.is_mergeable(c, _mergeCutoff))
			{
				foundMerge = true;
				m.merge(c);
				break;
			}
		}
		if (!foundMerge)
			merged.push_back(c);
	}
	return merged;
}

void Scanner::filter_candidates(std::vector<Anchor>& candidates) const
{
	// returns the best 4 candidates
	if (candidates.size() <= 4)
		return;

	std::sort(candidates.begin(), candidates.end(), size_sort());
	unsigned cutoff = 0;
	for (int i = 0; i < 4; ++i)
		cutoff += candidates[i].size();
	cutoff /= 8; // avg / 2

	int i = 0;
	for (; i < candidates.size(); ++i)
		if (candidates[i].size() < cutoff)
			break;
	if (i > 4)
		i = 4;
	if (i < candidates.size())
		candidates.resize(i);
}

bool Scanner::sort_top_to_bottom(std::vector<Anchor>& candidates)
{
	std::sort(candidates.begin(), candidates.end());
	if (candidates.size() < 4)
		return false;

	const Anchor& topLeft = candidates.front();
	std::vector<Anchor>::iterator p1_it = ++candidates.begin();
	std::vector<Anchor>::iterator p2_it = ++std::vector<Anchor>::iterator(p1_it);
	int p1_xoff = ::abs(p1_it->xavg() - topLeft.xavg());
	int p2_xoff = ::abs(p2_it->xavg() - topLeft.xavg());
	if (p2_xoff > p1_xoff)
		std::iter_swap(p1_it, p2_it);
	return true;
}

std::vector<Anchor> Scanner::scan()
{
	// scan horizontal
	std::vector<Anchor> candidates = t1_scan_rows<ScanState_114>();

	// for all horizontal results, scan vertical
	candidates = t2_scan_columns<ScanState_114>(candidates);

	// for all horizontal+vertical results, scan diagonal
	candidates = t3_scan_diagonal<ScanState_114>(candidates);

	// for all horizontal+vertical+diagonal results, do one more sanity check
	candidates = t4_confirm_scan<ScanState_114>(candidates);

	filter_candidates(candidates);
	sort_top_to_bottom(candidates);
	return candidates;
}

bool Scanner::chase_edge(const point<double>& start, const point<double>& unit) const
{
	// test 4 points. If we get 2/4, success
	int success = 0;
	for (int i : {-2, -1, 1, 2})
	{
		int x = start.x() + (unit.x() * i);
		int y = start.y() + (unit.y() * i);
		if (test_pixel(x, y))
			++success;
	}
	return success >= 2;
}

point<int> Scanner::find_edge(const point<int>& u, const point<int>& v, point<double> mid) const
{
	// should probably use more floats!
	point<double> distance_v = v.to_float() - u.to_float();
	point<double> distance_unit = distance_v / 512.0;
	point<double> out_v = {distance_v.y() / 64, distance_v.x() / -64};
	point<double> in_v = {-out_v.x(), -out_v.y()};

	if (mid == point<double>::NONE())
		mid = u.to_float() + (distance_v / 2.0);
	point<double> mid_point_anchor_adjust = out_v * (_anchorSize / 16.0);
	mid += mid_point_anchor_adjust;

	for (const point<double>& check : {out_v, in_v})
	{
		double max_check = std::max(abs(check.x()), abs(check.y()));
		point<double> unit = check / max_check;

		EdgeScanState state;
		double i = 0, j = 0;
		while (abs(i) <= abs(check.x()) and abs(j) <= abs(check.y()))
		{
			double x = mid.x() + i;
			double y = mid.y() + j;
			if (x < 0 or x >= _img.cols or y < 0 or y >= _img.rows)
			{
				i += unit.x();
				j += unit.y();
				continue;
			}
			bool active = test_pixel(x, y);
			int size = state.process(active);
			if (size > 0)
			{
				point<double> edge = {x - (unit.x() * size / 2), y - (unit.y() * size / 2)};
				if (chase_edge(edge, distance_unit))
					return edge.to_int();
			}
			i += unit.x();
			j += unit.y();
		}
	}

	return point<int>::NONE();
}

std::vector<point<int>> Scanner::scan_edges(const Corners& corners, Midpoints& mps) const
{
	std::vector<point<int>> edges;
	mps = Geometry::calculate_midpoints(corners);
	if (!mps)
		return edges;

	edges.push_back( find_edge(corners.top_left(), corners.top_right(), mps.top()) );
	edges.push_back( find_edge(corners.top_right(), corners.bottom_right(), mps.right()) );
	edges.push_back( find_edge(corners.bottom_right(), corners.bottom_left(), mps.bottom()) );
	edges.push_back( find_edge(corners.bottom_left(), corners.top_left(), mps.left()) );
	return edges;
}
