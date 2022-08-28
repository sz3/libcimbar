/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "Extractor.h"

#include "Deskewer.h"
#include "Scanner.h"
#include "cimb_translator/Config.h"
#include <vector>
using std::string;

Extractor::Extractor(unsigned image_size, unsigned anchor_size)
	: _imageSize(image_size? image_size : cimbar::Config::image_size())
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

	if ( !corners.is_granular_scale(de.image_size()) )
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

	if ( !corners.is_granular_scale(de.image_size()) )
		return NEEDS_SHARPEN;
	return SUCCESS;
}

int Extractor::extract(string read_path, cv::Mat& out)
{
	cv::Mat img = cv::imread(read_path);
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
	return extract(img, out);
}

int Extractor::extract(string read_path, string write_path)
{
	cv::UMat img = cv::imread(read_path).getUMat(cv::ACCESS_FAST); // cv::USAGE_ALLOCATE_SHARED_MEMORY would be nice...;
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

	int res = extract(img, img);

	cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
	cv::imwrite(write_path, img);
	return res;
}
