/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Deskewer.h"
#include "Scanner.h"
#include "util/vec_xy.h"

#include <opencv2/opencv.hpp>
#include <vector>

class Extractor
{
public:
	static constexpr int FAILURE = 0;
	static constexpr int SUCCESS = 1;
	static constexpr int NEEDS_SHARPEN = 2;

public:
	Extractor(unsigned padding=0, cimbar::vec_xy image_size={}, unsigned anchor_size=0);

	template <typename MAT>
	int extract(const MAT& img, MAT& out);

protected:
	cimbar::vec_xy _imageSize;
	unsigned _anchorSize;
	unsigned _padding;
};

template <typename MAT>
inline int Extractor::extract(const MAT& img, MAT& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de(_imageSize, _anchorSize);
	out = de.deskew(img, corners, _padding);

	if ( !corners.is_granular_scale(_imageSize) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}
