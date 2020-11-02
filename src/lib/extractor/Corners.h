/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Anchor.h"
#include "Point.h"
#include <opencv2/opencv.hpp>
#include <tuple>

class Corners
{
public:
	Corners(const std::vector<Anchor>& anchors)
	    : Corners(anchors[0].center(), anchors[1].center(), anchors[2].center(), anchors[3].center())
	{
	}

	Corners(point<int> top_left, point<int> top_right, point<int> bottom_left, point<int> bottom_right)
	    : _top_left(top_left)
	    , _top_right(top_right)
	    , _bottom_left(bottom_left)
	    , _bottom_right(bottom_right)
	{
	}

	const point<int>& top_left() const
	{
		return _top_left;
	}

	const point<int>& top_right() const
	{
		return _top_right;
	}

	const point<int>& bottom_right() const
	{
		return _bottom_right;
	}

	const point<int>& bottom_left() const
	{
		return _bottom_left;
	}

	std::vector<cv::Point2f> all() const
	{
		std::vector<cv::Point2f> points;
		points.push_back(cv::Point2f(_top_left.x(), _top_left.y()));
		points.push_back(cv::Point2f(_top_right.x(), _top_right.y()));
		points.push_back(cv::Point2f(_bottom_left.x(), _bottom_left.y()));
		points.push_back(cv::Point2f(_bottom_right.x(), _bottom_right.y()));
		return points;
	}

	bool is_granular_scale(unsigned min_size) const
	{
		// if any of our edges are < min_size, return false -- this means we'll be upscaling when we run a deskew.
		return (
		        check_scaling(_top_left, _top_right, min_size) and
		        check_scaling(_top_right, _bottom_right, min_size) and
		        check_scaling(_bottom_right, _bottom_left, min_size) and
		        check_scaling(_bottom_left, _top_left, min_size)
		);
	}

protected:
	bool check_scaling(const point<int>& a, const point<int>& b, unsigned min_size) const
	{
		return abs(a.x() - b.x()) > min_size or abs(a.y() - b.y()) > min_size;
	}

protected:
	point<int> _top_left;
	point<int> _top_right;
	point<int> _bottom_left;
	point<int> _bottom_right;
};
