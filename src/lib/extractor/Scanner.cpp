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

bool Scanner::scan_horizontal(std::vector<Anchor>& points, int y, int xstart, int xend) const
{
	if (xstart < 0)
		xstart = 0;
	if (xend < 0 or xend > _img.cols)
		xend = _img.cols;

	unsigned initCount = points.size();
	ScanState state = ScanState::create<ScanRatio::R114>();
	for (int x = xstart; x < xend; ++x)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x-res, x-1, y, y));
	}

	// if the pattern is at the edge of the range
	int res = state.process(false);
	if (res > 0)
	{
		int x = xend;
		points.push_back(Anchor(x-res, x-1, y, y));
	}
	return initCount != points.size();
}

bool Scanner::scan_vertical(std::vector<Anchor>& points, int x, int xmax, int ystart, int yend) const
{
	if (xmax < 0)
		xmax = x;
	int xavg = (x + xmax) / 2;

	if (ystart < 0)
		ystart = 0;
	if (yend < 0 or yend > _img.rows)
		yend = _img.rows;

	unsigned initCount = points.size();
	ScanState state = ScanState::create<ScanRatio::R114>();
	for (int y = ystart; y < yend; ++y)
	{
		bool active = test_pixel(xavg, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(xavg, xavg, y-res, y-1));
	}

	// if the pattern is at the edge of the range
	int res = state.process(false);
	if (res > 0)
	{
		int y = yend;
		points.push_back(Anchor(x, xmax, y-res, y-1));
	}
	return initCount != points.size();
}

bool Scanner::scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const
{
	xend = std::min(xend, _img.cols);
	yend = std::min(yend, _img.rows);

	// if we're up against the top/left bounds, roll the scan forward until we're inside the bounds
	if (xstart < 0)
	{
		int offset = -xstart;
		xstart += offset;
		ystart += offset;
	}
	if (ystart < 0)
	{
		int offset = -ystart;
		xstart += offset;
		ystart += offset;
	}

	// do the scan
	unsigned initCount = points.size();
	ScanState state = ScanState::create<ScanRatio::R114>();
	int x = xstart, y = ystart;
	for (; x < xend and y < yend; ++x, ++y)
	{
		bool active = test_pixel(x, y);
		int res = state.process(active);
		if (res > 0)
			points.push_back(Anchor(x-res, x-1, y-res, y-1));
	}

	// if the pattern is at the edge of the range
	int res = state.process(false);
	if (res > 0)
		points.push_back(Anchor(x-res, x-1, y-res, y-1));

	return initCount != points.size();
}

std::vector<Anchor> Scanner::t1_scan_rows() const
{
	std::vector<Anchor> points;
	for (int y = _skip; y < _img.rows; y += _skip)
		scan_horizontal(points, y);
	return points;
}

std::vector<Anchor> Scanner::t2_scan_columns(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		int ystart = p.y() - (3 * p.xrange());
		int yend = p.ymax() + (3 * p.xrange());
		scan_vertical(points, p.x(), p.xmax(), ystart, yend);
	}
	return points;
}

std::vector<Anchor> Scanner::t3_scan_diagonal(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		std::vector<Anchor> confirms;
		int xstart = p.xavg() - (2 * p.yrange());
		int xend = p.xavg() + (2 * p.yrange());
		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		if (!scan_diagonal(confirms, xstart, xend, ystart, yend))
			continue;

		Anchor merged(p);
		for (const Anchor& co : confirms)
		{
			if (co.is_mergeable(p, _mergeCutoff))
				merged.merge(co);
		}
		points.push_back(merged);
	}
	return points;
}

std::vector<Anchor> Scanner::t4_confirm_scan(const std::vector<Anchor>& candidates) const
{
	// because we have a lot of weird crap going on in the center of the image,
	// do one more scan of our (theoretical) anchor points.
	// this shouldn't be an issue for real anchors, but pretenders might get filtered out.
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		std::vector<Anchor> confirms;
		int xstart = p.x() - p.xrange();
		int xend = p.xmax() + p.xrange();
		if (!scan_horizontal(confirms, p.yavg(), xstart, xend))
			continue;

		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		if (!scan_vertical(confirms, p.xavg(), p.xavg(), ystart, yend))
			continue;

		Anchor merged(p);
		for (const Anchor& co : confirms)
		{
			if (co.is_mergeable(p, _mergeCutoff))
				merged.merge(co);
		}
		points.push_back(merged);
	}
	return deduplicate_candidates(points);
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
	std::vector<Anchor> candidates = t1_scan_rows();

	// for all horizontal results, scan vertical
	candidates = t2_scan_columns(candidates);

	// for all horizontal+vertical results, scan diagonal
	candidates = t3_scan_diagonal(candidates);

	// for all horizontal+vertical+diagonal results, do one more sanity check
	candidates = t4_confirm_scan(candidates);

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
