#pragma once

// env-ify
#include "rayliberation/render_texture.h"
#include <opencv2/opencv.hpp>

namespace cimbar {

	using image = cv::Mat;
	using tile = cv::Mat;

	inline bool imwrite(std::string filename, cv::Mat img)
	{
		// cv::imwrite expects BGR
		cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
		return cv::imwrite(filename, img);
	}

	inline bool imwrite(std::string filename, const cimbar::render_texture& img)
	{
		// TODO: something
		return true;
	}

}
