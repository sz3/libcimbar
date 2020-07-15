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
	Extractor();

	int extract(const cv::Mat& img, cv::Mat& out);
	int extract(const cv::UMat& img, cv::UMat& out);
	int extract(std::string read_path, cv::Mat& out);
	int extract(std::string read_path, std::string write_path);

protected:
};
