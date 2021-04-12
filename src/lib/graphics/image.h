#pragma once

#include <opencv2/opencv.hpp>

#ifdef LIBCIMBAR_USE_GLFW

#include "cimb_translator/Common.h"
#include "gui/window_glfw.h"

namespace cimbar {

	using frame = cv::Mat;
	using image = cv::Mat;

	inline bool imwrite(std::string filename, cv::Mat img)
	{
		// cv::imwrite expects BGR
		cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
		return cv::imwrite(filename, img);
	}

}
#else

#include "rayliberation/RayCommon.h"
#include "rayliberation/render_texture.h"
#include "rayliberation/texture.h"

namespace cimbar {

	using frame = cimbar::render_texture;
	using image = cimbar::texture;

	inline bool imwrite(std::string filename, const cimbar::render_texture& img)
	{
		// TODO: something
		return true;
	}

}
#endif

