/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Decoder.h"
#include "util/File.h"

#include <opencv2/opencv.hpp>
#include <string>

class DecoderPlus : public Decoder
{
public:
	using Decoder::Decoder;
	using Decoder::decode;

	unsigned decode(std::string filename, std::string output);

	bool load_ccm(std::string filename);
	bool save_ccm(std::string filename);
};

inline unsigned DecoderPlus::decode(std::string filename, std::string output)
{
	cv::Mat img = cv::imread(filename);
	cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

	std::ofstream f(output);
	return Decoder::decode(img, f, false);
}

inline bool DecoderPlus::load_ccm(std::string filename)
{
	File f(filename);
	std::string data = f.read_all();
	if (data.size() < 3*3*4)
		return false;

	cv::Mat temp(3, 3, CV_32F, data.data());

	_decoder.update_color_correction(temp);
	return true;
}

inline bool DecoderPlus::save_ccm(std::string filename)
{
	if (not _decoder.get_ccm().active())
		return false;

	cv::Mat temp(_decoder.get_ccm().mat());

	File f(filename, true);
	if (f.write(reinterpret_cast<const char*>(temp.data), temp.rows * temp.cols * temp.elemSize()) == 0)  // len will be 9*elemsize, but...
		return false;
	return true;
}

