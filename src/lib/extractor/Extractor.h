#pragma once

#include <opencv2/opencv.hpp>
#include <string>

class Extractor
{
public:
	Extractor();

	bool extract(const cv::Mat& img, cv::Mat& out);
	bool extract(std::string read_path, std::string write_path);

protected:
};
