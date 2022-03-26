/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Anchor.h"
#include "Point.h"
#include "ScanState.h"

#include <opencv2/opencv.hpp>
#include <functional>
#include <iostream>
#include <vector>

class Corners;
class Midpoints;

class Scanner
{
public: // public inline methods
	template <typename MAT>
	Scanner(const MAT& img, bool fast=true, bool dark=true, int skip=0);

	template <typename MAT, typename MAT2>
	static void threshold_fast(const MAT& img, MAT2& out);

	template <typename MAT, typename MAT2>
	static void threshold_adaptive(const MAT& img, MAT2& out);

	template <typename MAT>
	static cv::Mat preprocess_image(const MAT& img, bool fast);

	template <typename MAT, typename MAT2>
	static void preprocess_image(const MAT& img, MAT2& out, bool fast);

	static unsigned nextPowerOfTwoPlusOne(unsigned v); // helper

	// rest of public interface
	std::vector<Anchor> scan();
	std::vector<point<int>> scan_edges(const Corners& corners, Midpoints& mps) const;
	int anchor_size() const;

public: // other interesting methods
	std::vector<Anchor> deduplicate_candidates(const std::vector<Anchor>& candidates) const;
	unsigned filter_candidates(std::vector<Anchor>& candidates) const;
	static bool sort_top_to_bottom(std::vector<Anchor>& anchors);

	template <typename SCANTYPE>
	void t1_scan_rows(std::function<void(const Anchor&)> fun, int skip=-1, int y=-1, int yend=-1, int xstart=-1, int xend=-1) const;

	template <typename SCANTYPE>
	void t2_scan_column(const Anchor& hint, std::function<void(const Anchor&)> fun) const;

	template <typename SCANTYPE>
	void t3_scan_diagonal(const Anchor& hint, std::function<void(const Anchor&)> funs) const;

	template <typename SCANTYPE>
	void t4_confirm_scan(Anchor hint, bool merge_confirms, std::function<void(const Anchor&)> fun) const;

	unsigned scan_primary(std::vector<Anchor>& candidates);
	bool add_bottom_right_corner(std::vector<Anchor>& anchors, unsigned cutoff);

protected: // internal member functions
	bool test_pixel(int x, int y) const;

	template <typename SCANTYPE>
	bool scan_horizontal(std::vector<Anchor>& points, int y, int xstart=-1, int xend=-1) const;

	template <typename SCANTYPE>
	bool scan_vertical(std::vector<Anchor>& points, int x, int xmax=-1, int ystart=-1, int yend=-1) const;

	template <typename SCANTYPE>
	bool scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const;

	template <typename SCANTYPE>
	void on_t1_scan(const Anchor& found, std::vector<Anchor>& candidates, bool merge_confirms) const;

	// edge detection
	bool chase_edge(const point<double>& start, const point<double>& unit) const;
	point<int> find_edge(const point<int>& u, const point<int>& v, point<double> mid) const;

protected:
	cv::Mat _img;
	bool _dark;
	int _skip;
	int _mergeCutoff;
	int _anchorSize;
};

inline unsigned Scanner::nextPowerOfTwoPlusOne(unsigned v)
{
	// get next power of 2 + 1
	// min 3
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return std::max(3U, v + 2);
}

template <typename MAT, typename MAT2>
inline void Scanner::threshold_fast(const MAT& img, MAT2& out)
{
	cv::threshold(img, out, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
}

template <typename MAT, typename MAT2>
inline void Scanner::threshold_adaptive(const MAT& img, MAT2& out)
{
	unsigned unit = std::min(img.cols, img.rows);
	unit = nextPowerOfTwoPlusOne((unsigned)(unit * 0.05));
	cv::adaptiveThreshold(img, out, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, unit, -10);
}

template <typename MAT>
inline cv::Mat Scanner::preprocess_image(const MAT& img, bool fast)
{
	cv::Mat out;
	preprocess_image(img, out, fast);
	return out;
}

template <typename MAT, typename MAT2>
inline void Scanner::preprocess_image(const MAT& img, MAT2& out, bool fast)
{
	MAT temp;
	if (img.channels() >= 3)
		cv::cvtColor(img, temp, cv::COLOR_RGB2GRAY);
	else
		temp = img.clone();

	unsigned unit = std::min(img.cols, img.rows);
	unit = std::max(nextPowerOfTwoPlusOne((unsigned)(unit * 0.002)), 3U);
	cv::GaussianBlur(temp, temp, cv::Size(unit, unit), 0);

	if (fast)
		threshold_fast(temp, out);
	else
		threshold_adaptive(temp, out);
}

template <typename MAT>
inline Scanner::Scanner(const MAT& img, bool fast, bool dark, int skip)
    : _dark(dark)
    , _skip(skip? skip : std::min(img.rows, img.cols) / 60)
    , _mergeCutoff(img.cols / 30)
    , _anchorSize(30)
{
	_img = preprocess_image(img, fast);
}

template <typename SCANTYPE>
inline bool Scanner::scan_horizontal(std::vector<Anchor>& points, int y, int xstart, int xend) const
{
	if (xstart < 0)
		xstart = 0;
	if (xend < 0 or xend > _img.cols)
		xend = _img.cols;

	unsigned initCount = points.size();
	SCANTYPE state;
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

template <typename SCANTYPE>
inline bool Scanner::scan_vertical(std::vector<Anchor>& points, int x, int xmax, int ystart, int yend) const
{
	if (xmax < 0)
		xmax = x;
	int xavg = (x + xmax) / 2;

	if (ystart < 0)
		ystart = 0;
	if (yend < 0 or yend > _img.rows)
		yend = _img.rows;

	unsigned initCount = points.size();
	SCANTYPE state;
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
		points.push_back(Anchor(xavg, xavg, y-res, y-1));
	}
	return initCount != points.size();
}

template <typename SCANTYPE>
inline bool Scanner::scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const
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
	SCANTYPE state;
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

template <typename SCANTYPE>
inline void Scanner::t1_scan_rows(std::function<void(const Anchor&)> fun, int skip, int y, int yend, int xstart, int xend) const
{
	if (skip <= 0)
		skip = _skip;
	if (y < 0)
		y = skip;
	if (yend < 0 or yend > _img.rows)
		yend = _img.rows;

	std::vector<Anchor> points;
	for (; y < yend; y += skip)
		scan_horizontal<SCANTYPE>(points, y, xstart, xend);

	for (const Anchor& p : points)
		fun(p);
}

template <typename SCANTYPE>
inline void Scanner::t2_scan_column(const Anchor& hint, std::function<void(const Anchor&)> fun) const
{
	std::vector<Anchor> points;
	int ystart = hint.y() - (3 * hint.xrange());
	int yend = hint.ymax() + (3 * hint.xrange());
	scan_vertical<SCANTYPE>(points, hint.x(), hint.xmax(), ystart, yend);

	for (const Anchor& p : points)
		fun(p);
}

template <typename SCANTYPE>
inline void Scanner::t3_scan_diagonal(const Anchor& hint, std::function<void(const Anchor&)> fun) const
{
	std::vector<Anchor> confirms;
	int xstart = hint.xavg() - (2 * hint.yrange());
	int xend = hint.xavg() + (2 * hint.yrange());
	int ystart = hint.y() - hint.yrange();
	int yend = hint.ymax() + hint.yrange();
	if (!scan_diagonal<SCANTYPE>(confirms, xstart, xend, ystart, yend))
		return;

	bool confirm = false;
	Anchor merged(hint);
	for (const Anchor& co : confirms)
	{
		if (co.is_mergeable(hint, _mergeCutoff))
		{
			confirm = true;
			merged.merge(co);
		}
	}
	if (confirm)
		fun(merged);
}

template <typename SCANTYPE>
inline void Scanner::t4_confirm_scan(Anchor hint, bool merge_confirms, std::function<void(const Anchor&)> fun) const
{
	// because we have a lot of weird crap going on in the center of the image,
	// do one more scan of our (theoretical) anchor points.
	// this shouldn't be an issue for real anchors, but pretenders might get filtered out.

	// probably should do multiple sets of confirm checks, rather than just the one.
	// validate on various input samples though!
	{
		std::vector<Anchor> confirms;
		int xstart = hint.x() - hint.xrange();
		int xend = hint.xmax() + hint.xrange();
		int yavg = hint.yavg();
		for (int y : {yavg - 1, yavg, yavg + 1})
			if (!scan_horizontal<SCANTYPE>(confirms, y, xstart, xend))
				return;

		bool confirm = false;
		for (const Anchor& co : confirms)
		{
			if (co.is_mergeable(hint, _mergeCutoff))
			{
				confirm = true;
				if (!merge_confirms)
					break;
				hint.merge(co);
			}
		}
		if (!confirm)
			return;
	}

	{
		std::vector<Anchor> confirms;
		int ystart = hint.y() - hint.yrange();
		int yend = hint.ymax() + hint.yrange();
		int xavg = hint.xavg();
		for (int x : {xavg - 1, xavg, xavg + 1})
			if (!scan_vertical<SCANTYPE>(confirms, x, x, ystart, yend))
				return;

		bool confirm = false;
		for (const Anchor& co : confirms)
		{
			if (co.is_mergeable(hint, _mergeCutoff))
			{
				confirm = true;
				if (!merge_confirms)
					break;
				hint.merge(co);
			}
		}
		if (!confirm)
			return;
	}

	fun(hint);
}

template <typename SCANTYPE>
inline void Scanner::on_t1_scan(const Anchor& found, std::vector<Anchor>& candidates, bool merge_confirms) const
{
	for (const Anchor& c : candidates)
		if (c.is_mergeable(found, _mergeCutoff))
			return;

	t2_scan_column<SCANTYPE>(found, [&] (const Anchor& p) {
		t3_scan_diagonal<SCANTYPE>(p, [&] (const Anchor& p) {
			t4_confirm_scan<SCANTYPE>(p, merge_confirms, [&] (const Anchor& p) {
				candidates.push_back(p);
			});
		});
	});
}
