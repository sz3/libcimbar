/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Extractor.h"

#include "Deskewer.h"
#include "Scanner.h"
#include "cimb_translator/Config.h"
#include <vector>
using std::string;

Extractor::Extractor(cimbar::vec_xy image_size, unsigned anchor_size)
	: _imageSize({
		image_size.width()? image_size.width() : cimbar::Config::image_size_x(),
		image_size.height()? image_size.height() : cimbar::Config::image_size_y()})
	, _anchorSize(anchor_size? anchor_size : cimbar::Config::anchor_size())
{
}

int Extractor::extract(const cv::Mat& img, cv::Mat& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de(_imageSize, _anchorSize);
	out = de.deskew(img, corners);

	if ( !corners.is_granular_scale(_imageSize) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}

int Extractor::extract(const cv::UMat& img, cv::UMat& out)
{
	Scanner scanner(img);
	std::vector<Anchor> points = scanner.scan();
	if (points.size() < 4)
		return FAILURE;

	Corners corners(points);
	Deskewer de(_imageSize, _anchorSize);
	out = de.deskew(img, corners);

	if ( !corners.is_granular_scale(_imageSize) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}
