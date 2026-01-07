/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Corners.h"

#include "util/vec_xy.h"
#include <opencv2/opencv.hpp>

#include <vector>

class Deskewer
{
public:
	Deskewer(cimbar::vec_xy image_size={}, unsigned anchor_size=0);

	template <typename MAT>
	MAT deskew(const MAT& img, const Corners& corners, unsigned padding=0);

protected:
	cimbar::vec_xy _imageSize;
	unsigned _anchorSize;
};

template <typename MAT>
inline MAT Deskewer::deskew(const MAT& img, const Corners& corners, unsigned padding)
{
	std::vector<cv::Point2f> outputPoints;
	outputPoints.push_back(cv::Point2f(_anchorSize+padding, _anchorSize+padding));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize+padding, _anchorSize+padding));
	outputPoints.push_back(cv::Point2f(_anchorSize+padding, _imageSize.height() - _anchorSize+padding));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize+padding, _imageSize.height() - _anchorSize+padding));

	// + 2*padding ?
	MAT output(_imageSize.height() + (padding*2), _imageSize.width() + (padding*2), img.type());
	cv::Mat transform = cv::getPerspectiveTransform(corners.all(), outputPoints);

	cv::warpPerspective(img, output, transform, output.size(), cv::INTER_LINEAR);
	return output;
}
