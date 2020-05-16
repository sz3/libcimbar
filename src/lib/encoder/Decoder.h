#pragma once

#include "cimb_translator/CimbDecoder.h"
#include <opencv2/opencv.hpp>
#include <string>

class Decoder
{
public:
	Decoder(unsigned ecc_bytes=15, unsigned bits_per_op=0);

	unsigned decode(const cv::Mat& img, std::string output);
	unsigned decode(std::string filename, std::string output);

protected:
	unsigned _eccBytes;
	unsigned _bitsPerOp;
	CimbDecoder _decoder;
};
