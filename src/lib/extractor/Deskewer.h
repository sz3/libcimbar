/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Corners.h"

#include "util/vec_xy.h"
#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

class Deskewer
{
public:
	Deskewer(cimbar::vec_xy image_size={}, unsigned anchor_size=0);

	template <typename MAT>
	MAT deskew(const MAT& img, const Corners& corners);

	cv::Mat deskew(std::string img, const Corners& corners);
	bool save(const cv::Mat& img, std::string path);

protected:
	cimbar::vec_xy _imageSize;
	int _anchorSize;
};

template <typename MAT>
inline MAT Deskewer::deskew(const MAT& img, const Corners& corners)
{
	std::vector<cv::Point2f> outputPoints;
	outputPoints.push_back(cv::Point2f(_anchorSize, _anchorSize));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize, _anchorSize));
	outputPoints.push_back(cv::Point2f(_anchorSize, _imageSize.height() - _anchorSize));
	outputPoints.push_back(cv::Point2f(_imageSize.width() - _anchorSize, _imageSize.height() - _anchorSize));

	MAT output(_imageSize.height(), _imageSize.width(), img.type());
	cv::Mat transform = cv::getPerspectiveTransform(corners.all(), outputPoints);

	cv::warpPerspective(img, output, transform, output.size(), cv::INTER_LINEAR);
	return output;
}
