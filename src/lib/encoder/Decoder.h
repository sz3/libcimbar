#pragma once

#include <opencv2/opencv.hpp>
#include <string>

class Decoder
{
public:
	Decoder(unsigned bits_per_op=0);

	unsigned decode(const cv::Mat& img, std::string output);
	unsigned decode(std::string filename, std::string output);

protected:
	unsigned _bits_per_op;
};
