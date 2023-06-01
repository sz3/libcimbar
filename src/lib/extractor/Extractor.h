/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <opencv2/opencv.hpp>
#include <string>

class Extractor
{
public:
	static constexpr int FAILURE = 0;
	static constexpr int SUCCESS = 1;
	static constexpr int NEEDS_SHARPEN = 2;

public:
	Extractor(unsigned image_size=0, unsigned anchor_size=0);

	int extract(const cv::Mat& img, cv::Mat& out);
	int extract(const cv::UMat& img, cv::UMat& out);
	int extract(std::string read_path, cv::Mat& out);
	int extract(std::string read_path, std::string write_path);

protected:
	unsigned _imageSize;
	unsigned _anchorSize;
};
