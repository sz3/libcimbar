/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimb_translator/Config.h"
#include "encoder/Encoder.h"
#include "fountain/FountainInit.h"
#include "gui/window_cvhighgui.h"
#include "gui/window_glfw.h"
#include "serialize/str.h"
#include "util/loop_iterator.h"

#include "cxxopts/cxxopts.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;


int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar video encoder", "Draw a bunch of weird static on the screen!");

	int compressionLevel = cimbar::Config::compression_level();
	unsigned ecc = cimbar::Config::ecc_bytes();
	unsigned defaultFps = 15;
	options.add_options()
	    ("i,in", "Source file", cxxopts::value<vector<string>>())
	    ("c,compression", "Compression level. 0 == no compression.", cxxopts::value<int>()->default_value(turbo::str::str(compressionLevel)))
	    ("e,ecc", "ECC level", cxxopts::value<unsigned>()->default_value(turbo::str::str(ecc)))
	    ("f,fps", "Target FPS", cxxopts::value<unsigned>()->default_value(turbo::str::str(defaultFps)))
	    ("s,shakycam", "Successive images are offset, like a shaky camera effect", cxxopts::value<bool>())
	    ("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in"});
	options.positional_help("<in...>");

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	vector<string> infiles = result["in"].as<vector<string>>();

	compressionLevel = result["compression"].as<int>();
	ecc = result["ecc"].as<unsigned>();
	unsigned fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = defaultFps;
	unsigned delay = 1000 / fps;

	FountainInit::init();

	bool dark = true;
	cv::Scalar bgcolor = dark? cv::Scalar(0, 0, 0) : cv::Scalar(0xFF, 0xFF, 0xFF);
	cv::Mat windowImg = cv::Mat(1080, 1080, CV_8UC3, bgcolor);

	bool running = true;
	bool start = true;

	bool shakycam = result.count("shakycam");
	std::array<std::pair<int, int>, 8> shakePos = {{
	    {0, 0}, {-8, -8}, {0, 0}, {8, 8}, {0, 0}, {-8, 8}, {0, 0}, {8, -8}
	}};
	loop_iterator shakeIt(shakePos);

#ifdef LIBCIMBAR_USE_GLFW
	window_glfw w(1080, 1080, "cimbar_send");
	if (!w.is_good())
	{
		std::cerr << "failed to open GL window :(" << std::endl;
		return 50;
	}
#else
	window_cvhighgui w;
#endif

	auto draw = [&windowImg, delay, bgcolor, shakycam, &running, &start, &shakeIt, &w] (const cv::Mat& frame, unsigned) {
		if (!start and w.should_close())
			return running = false;
		start = false;

		int offsetX = 28;
		int offsetY = 28;
		if (shakycam)
		{
			windowImg = bgcolor;
			if (++shakeIt)
			{
				offsetX += (*shakeIt).first;
				offsetY += (*shakeIt).second;
			}
		}
		frame.copyTo(windowImg(cv::Rect(offsetX, offsetY, frame.cols, frame.rows)));

		w.show(windowImg, delay);
		return true;
	};

	Encoder en(ecc);
	while (running)
		for (const string& f : infiles)
			en.encode_fountain(f, draw, compressionLevel);
	return 0;
}
