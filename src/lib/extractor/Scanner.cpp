/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Scanner.h"

#include "Corners.h"
#include "EdgeScanState.h"
#include "Geometry.h"
#include "ScanState.h"
#include <algorithm>

namespace {
	struct size_sort
	{
		inline bool operator() (const Anchor& an1, const Anchor& an2)
		{
			return an1.size() > an2.size();
		}
	};

	template <int N>
	int fix_index(int i)
	{
		if (i < 0)
			i = N-1;
		else if (i >= N)
			i = 0;
		return i;
	}

	int get_longest_edge(const std::vector<point<int>>& edges)
	{
		int longest = 0;
		int max_d = 0;
		for (unsigned i = 0; i < edges.size(); ++i)
		{
			int d = edges[i].dot(edges[i]);
			if (d > max_d)
			{
				longest = i;
				max_d = d;
			}
		}
		return longest;
	}
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

unsigned Scanner::filter_candidates(std::vector<Anchor>& candidates) const
{
	// returns the best 3 candidates
	if (candidates.size() < 3)
		return 0;

	std::sort(candidates.begin(), candidates.end(), size_sort());
	unsigned cutoff = 0;
	for (int i = 0; i < 3; ++i)
		cutoff += candidates[i].size();
	cutoff /= 8; // avg / 2

	unsigned i = 0;
	for (; i < candidates.size(); ++i)
		if (candidates[i].size() < cutoff)
			break;
	if (i > 3)
		i = 3;
	if (i < candidates.size())
		candidates.resize(i);

	return cutoff;
}

bool Scanner::sort_top_to_bottom(std::vector<Anchor>& anchors)
{
	if (anchors.size() < 3)
		return false;

	std::vector<point<int>> edges({
	    anchors[1].center() - anchors[2].center(),
	    anchors[2].center() - anchors[0].center(),
	    anchors[0].center() - anchors[1].center(),
	});

	// because of how we ordered our edges, the index of the longest edge is also the index of the anchor opposite it.
	int top_left = get_longest_edge(edges);
	int top_right, bottom_left;

	// now, we need to find the order of the other two:
	const point<int>& departing_edge = edges[fix_index<3>(top_left - 1)];
	point<int> incoming_edge = edges[fix_index<3>(top_left + 1)];
	incoming_edge = {-incoming_edge.y(), incoming_edge.x()}; // rotate
	point<int> overlap = departing_edge - incoming_edge;

	if (overlap.dot(overlap) < departing_edge.dot(departing_edge))
	{
		top_right = fix_index<3>(top_left + 1);
		bottom_left = fix_index<3>(top_left - 1);
	}
	else
	{
		top_right = fix_index<3>(top_left - 1);
		bottom_left = fix_index<3>(top_left + 1);
	}

	// apply the order.
	anchors = {anchors[top_left], anchors[top_right], anchors[bottom_left]};
	return true;
}

bool Scanner::add_bottom_right_corner(std::vector<Anchor>& anchors, unsigned cutoff)
{
	double topScalar = anchors[2].max_range() / std::max<double>(anchors[1].max_range(), anchors[0].max_range());
	point<int> topEdge = (anchors[1].center() - anchors[0].center()) * topScalar;
	point<int> guess1 = anchors[2].center() + topEdge;

	double leftScalar = anchors[1].max_range() / std::max<double>(anchors[2].max_range(), anchors[0].max_range());
	point<int> leftEdge = (anchors[2].center() - anchors[0].center()) * leftScalar;
	point<int> guess2 = anchors[1].center() + leftEdge;

	point<int> center = (guess1 + guess2) / 2;

	// the scan area
	float uncertainty = 2;
	int range = std::max({anchors[0].max_range(), anchors[1].max_range(), anchors[2].max_range()}) * uncertainty;

	// the secondary anchor center is about half the size of the primary one. So we need a 2x granular search.
	int skip = _skip / 2;

	int ystart = center.y() - range;
	int yend = center.y() + range;
	int xstart = center.x() - range;
	int xend = center.x() + range;

	std::vector<Anchor> candidates;
	t1_scan_rows<ScanState_122>([&] (const Anchor& p) {
		on_t1_scan<ScanState_122>(p, candidates, false);
	}, skip, ystart, yend, xstart, xend);

	if (candidates.size() == 0)
		return false;

	for (const Anchor& c : candidates)
		if (c.size() > cutoff)
		{
			anchors.push_back(c);
			return true;
		}
	return false;
}

unsigned Scanner::scan_primary(std::vector<Anchor>& candidates)
{
	t1_scan_rows<ScanState_114>([&] (const Anchor& p) {
		on_t1_scan<ScanState_114>(p, candidates, true);
	});

	unsigned cutoff = filter_candidates(candidates);
	sort_top_to_bottom(candidates);
	return cutoff;
}

std::vector<Anchor> Scanner::scan()
{
	std::vector<Anchor> candidates;
	unsigned cutoff = scan_primary(candidates);

	if (candidates.size() == 3 and cutoff != 0)
		add_bottom_right_corner(candidates, cutoff);
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
