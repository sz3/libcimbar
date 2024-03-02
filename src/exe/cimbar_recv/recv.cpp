/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimb_translator/Config.h"
#include "compression/zstd_decompressor.h"
#include "encoder/Decoder.h"
#include "extractor/Extractor.h"
#include "fountain/fountain_decoder_sink.h"
#include "gui/window_glfw.h"

#include "cxxopts/cxxopts.hpp"
#include "serialize/str.h"

#include <GLFW/glfw3.h>
#include <opencv2/videoio.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
using std::string;

namespace {

	template <typename TP>
	TP wait_for_frame_time(unsigned delay, const TP& start)
	{
		unsigned millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		if (delay > millis)
			std::this_thread::sleep_for(std::chrono::milliseconds(delay-millis));
		return std::chrono::high_resolution_clock::now();
	}
}


int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar video decoder", "Use the camera to decode data!");

	unsigned colorBits = cimbar::Config::color_bits();
	unsigned ecc = cimbar::Config::ecc_bytes();
	unsigned defaultFps = 60;
	options.add_options()
		("i,in", "Video source.", cxxopts::value<string>())
		("o,out", "Output directory (decoding).", cxxopts::value<string>())
		("c,colorbits", "Color bits. [0-3]", cxxopts::value<int>()->default_value(turbo::str::str(colorBits)))
		("e,ecc", "ECC level", cxxopts::value<unsigned>()->default_value(turbo::str::str(ecc)))
		("f,fps", "Target decode FPS", cxxopts::value<unsigned>()->default_value(turbo::str::str(defaultFps)))
		("m,mode", "Select a cimbar mode. B (the default) is new to 0.6.x. 4C is the 0.5.x config. [B,4C]", cxxopts::value<string>()->default_value("B"))
		("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in", "out"});
	options.positional_help("<in> <out>");

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("in") or !result.count("out"))
	{
	  std::cout << options.help() << std::endl;
	  return 0;
	}

	string source = result["in"].as<string>();
	string outpath = result["out"].as<string>();

	colorBits = std::min(3, result["colorbits"].as<int>());
	ecc = result["ecc"].as<unsigned>();

	bool legacy_mode = false;
	if (result.count("mode"))
	{
		string mode = result["mode"].as<string>();
		legacy_mode = (mode == "4c") or (mode == "4C");
	}
	unsigned color_mode = legacy_mode? 0 : 1;

	unsigned fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = defaultFps;
	unsigned delay = 1000 / fps;

	cv::VideoCapture vc(source.c_str());
	if (!vc.isOpened())
	{
		std::cerr << "failed to open video device :(" << std::endl;
		return 70;
	}
	vc.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
	vc.set(cv::CAP_PROP_FRAME_HEIGHT, 1200);
	vc.set(cv::CAP_PROP_FPS, fps);

	// set max camera res, and use aspect ratio for window size...

	std::cout << fmt::format("width: {}, height {}, exposure {}", vc.get(cv::CAP_PROP_FRAME_WIDTH), vc.get(cv::CAP_PROP_FRAME_HEIGHT), vc.get(cv::CAP_PROP_EXPOSURE)) << std::endl;

	double ratio = vc.get(cv::CAP_PROP_FRAME_WIDTH) / vc.get(cv::CAP_PROP_FRAME_HEIGHT);
	int height = 600;
	int width = height * ratio;
	std::cout << "got dimensions " << width << "," << height << std::endl;

	cimbar::window_glfw window(width, height, "cimbar_recv");
	if (!window.is_good())
	{
		std::cerr << "failed to create window :(" << std::endl;
		return 70;
	}
	window.auto_scale_to_window();

	Extractor ext;
	Decoder dec(-1, -1);

	unsigned chunkSize = cimbar::Config::fountain_chunk_size(ecc, colorBits+cimbar::Config::symbol_bits(), legacy_mode);
	fountain_decoder_sink<cimbar::zstd_decompressor<std::ofstream>> sink(outpath, chunkSize);

	cv::Mat mat;

	unsigned count = 0;
	std::chrono::time_point start = std::chrono::high_resolution_clock::now();
	while (true)
	{
		++count;

		// delay, then try to read frame
		start = wait_for_frame_time(delay, start);
		if (window.should_close())
			break;

		if (!vc.read(mat))
		{
			std::cerr << "failed to read from cam" << std::endl;
			continue;
		}

		cv::UMat img = mat.getUMat(cv::ACCESS_RW);
		cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);

		// draw some stats on mat?
		window.show(mat, 0);

		// extract
		bool shouldPreprocess = true;
		int res = ext.extract(img, img);
		if (!res)
		{
			//std::cerr << "no extract " << mat.cols << "," << mat.rows << std::endl;
			continue;
		}
		else if (res == Extractor::NEEDS_SHARPEN)
			shouldPreprocess = true;

		// decode
		int bytes = dec.decode_fountain(img, sink, color_mode, shouldPreprocess);
		if (bytes > 0)
			std::cerr << "got some bytes " << bytes << std::endl;
	}

	return 0;
}
