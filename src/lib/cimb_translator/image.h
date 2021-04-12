#pragma once

#include <opencv2/opencv.hpp>

#ifdef LIBCIMBAR_USE_RAYLIB

#include "rayliberation/RayCommon.h"
#include "rayliberation/render_texture.h"
#include "rayliberation/texture.h"

namespace cimbar {

	using frame = cimbar::render_texture;
	using image = cimbar::texture;

	template <typename CTEX>
	inline bool imwrite(std::string filename, const CTEX& img)
	{
		Image temp = img.screenshot();
		bool res = ExportImage(temp, filename.c_str());
		UnloadImage(temp);
		return res;
	}

}

#else

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

#endif

