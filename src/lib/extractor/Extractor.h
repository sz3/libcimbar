/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "SimpleExtractor.h"

#include <opencv2/opencv.hpp>
#include <string>

class Extractor : public SimpleExtractor
{
public:
	using SimpleExtractor::SimpleExtractor;
	using SimpleExtractor::extract;

	int extract(std::string read_path, cv::Mat& out);
	int extract(std::string read_path, std::string write_path);

protected:
};

inline int Extractor::extract(std::string read_path, cv::Mat& out)
{
	cv::Mat img = cv::imread(read_path);
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
	return SimpleExtractor::extract(img, out);
}

inline int Extractor::extract(std::string read_path, std::string write_path)
{
	cv::UMat img = cv::imread(read_path).getUMat(cv::ACCESS_FAST); // cv::USAGE_ALLOCATE_SHARED_MEMORY would be nice...;
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

	int res = SimpleExtractor::extract(img, img);

	cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
	cv::imwrite(write_path, img);
	return res;
}
