#pragma once

#include "Anchor.h"
#include "Point.h"
#include "ScanState.h"

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

class Corners;
class Midpoints;

class Scanner
{
public: // public inline methods
	template <typename MAT>
	Scanner(const MAT& img, bool dark=true, int skip=0);

	template <typename MAT>
	static cv::Mat threshold_clahe(const MAT& img);

	template <typename MAT>
	static cv::Mat threshold_fast(const MAT& img);

	template <typename MAT>
	static cv::Mat preprocess_image(const MAT& img, bool quick=true);

	static unsigned nextPowerOfTwoPlusOne(unsigned v); // helper

	// rest of public interface
	std::vector<Anchor> scan();
	std::vector<point<int>> scan_edges(const Corners& corners, Midpoints& mps) const;
	int anchor_size() const;

public: // other interesting methods
	std::vector<Anchor> deduplicate_candidates(const std::vector<Anchor>& candidates) const;
	void filter_candidates(std::vector<Anchor>& candidates) const;

	template <typename SCANTYPE>
	std::vector<Anchor> t1_scan_rows(int skip=-1, int y=-1, int yend=-1, int xstart=-1, int xend=-1) const;

	template <typename SCANTYPE>
	std::vector<Anchor> t2_scan_columns(const std::vector<Anchor>& candidates) const;

	template <typename SCANTYPE>
	std::vector<Anchor> t3_scan_diagonal(const std::vector<Anchor>& candidates) const;

	template <typename SCANTYPE>
	std::vector<Anchor> t4_confirm_scan(const std::vector<Anchor>& candidates) const;

	bool sort_top_to_bottom(std::vector<Anchor>& points);

protected: // internal member functions
	bool test_pixel(int x, int y) const;

	template <typename SCANTYPE>
	bool scan_horizontal(std::vector<Anchor>& points, int y, int xstart=-1, int xend=-1) const;

	template <typename SCANTYPE>
	bool scan_vertical(std::vector<Anchor>& points, int x, int xmax=-1, int ystart=-1, int yend=-1) const;

	template <typename SCANTYPE>
	bool scan_diagonal(std::vector<Anchor>& points, int xstart, int xend, int ystart, int yend) const;

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

template <typename MAT>
inline cv::Mat Scanner::threshold_clahe(const MAT& img)
{
	cv::Mat res;
	cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(4.0, cv::Size(100, 100));
	clahe->apply(img, res);

	cv::threshold(res, res, 127, 255, cv::THRESH_BINARY);
	return res;
}

template <typename MAT>
inline cv::Mat Scanner::threshold_fast(const MAT& img)
{
	unsigned unit = std::min(img.cols, img.rows);
	unit = nextPowerOfTwoPlusOne((unsigned)(unit * 0.05));
	cv::Mat res;
	cv::adaptiveThreshold(img, res, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, unit, 0);
	return res;
}

template <typename MAT>
inline cv::Mat Scanner::preprocess_image(const MAT& img, bool quick)
{
	unsigned unitX = nextPowerOfTwoPlusOne((unsigned)(img.cols * 0.002));
	unsigned unitY = nextPowerOfTwoPlusOne((unsigned)(img.rows * 0.002));

	MAT out;
	if (img.channels() >= 3)
		cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
	else
		out = img.clone();

	cv::GaussianBlur(out, out, cv::Size(unitY, unitX), 0);
	if (quick)
		return threshold_fast(out);
	else
		return threshold_clahe(out);
}

template <typename MAT>
inline Scanner::Scanner(const MAT& img, bool dark, int skip)
    : _dark(dark)
    , _skip(skip? skip : std::min(img.rows, img.cols) / 60)
    , _mergeCutoff(img.cols / 30)
    , _anchorSize(30)
{
	_img = preprocess_image(img);
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
		points.push_back(Anchor(x, xmax, y-res, y-1));
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
inline std::vector<Anchor> Scanner::t1_scan_rows(int skip, int y, int yend, int xstart, int xend) const
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
	return points;
}

template <typename SCANTYPE>
inline std::vector<Anchor> Scanner::t2_scan_columns(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		int ystart = p.y() - (3 * p.xrange());
		int yend = p.ymax() + (3 * p.xrange());
		scan_vertical<SCANTYPE>(points, p.x(), p.xmax(), ystart, yend);
	}
	return points;
}

template <typename SCANTYPE>
inline std::vector<Anchor> Scanner::t3_scan_diagonal(const std::vector<Anchor>& candidates) const
{
	std::vector<Anchor> points;
	for (const Anchor& p : candidates)
	{
		std::vector<Anchor> confirms;
		int xstart = p.xavg() - (2 * p.yrange());
		int xend = p.xavg() + (2 * p.yrange());
		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		if (!scan_diagonal<SCANTYPE>(confirms, xstart, xend, ystart, yend))
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

template <typename SCANTYPE>
inline std::vector<Anchor> Scanner::t4_confirm_scan(const std::vector<Anchor>& candidates) const
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
		if (!scan_horizontal<SCANTYPE>(confirms, p.yavg(), xstart, xend))
			continue;

		int ystart = p.y() - p.yrange();
		int yend = p.ymax() + p.yrange();
		if (!scan_vertical<SCANTYPE>(confirms, p.xavg(), p.xavg(), ystart, yend))
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
