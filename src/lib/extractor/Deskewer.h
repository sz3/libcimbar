/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Corners.h"

#include "util/vec_xy.h"
#include <opencv2/opencv.hpp>

#include <vector>

class Deskewer
{
public:
	Deskewer(unsigned padding=0, cimbar::vec_xy image_size={}, unsigned anchor_size=0);

	template <typename MAT>
	MAT deskew(const MAT& img, const Corners& corners);

protected:
	cimbar::vec_xy _imageSize;
	unsigned _anchorSize;
	unsigned _padding;
};

template <typename MAT>
inline MAT Deskewer::deskew(const MAT& img, const Corners& corners)
{
	std::vector<cv::Point2f> outputPoints;
	outputPoints.push_back(cv::Point2f(_anchorSize+_padding, _anchorSize+_padding));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize+_padding, _anchorSize+_padding));
	outputPoints.push_back(cv::Point2f(_anchorSize+_padding, _imageSize.height() - _anchorSize+_padding));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize+_padding, _imageSize.height() - _anchorSize+_padding));

	// + 2*padding ?
	MAT output(_imageSize.height() + (_padding*2), _imageSize.width() + (_padding*2), img.type());
	cv::Mat transform = cv::getPerspectiveTransform(corners.all(), outputPoints);

	cv::warpPerspective(img, output, transform, output.size(), cv::INTER_LINEAR);
	return output;
}
