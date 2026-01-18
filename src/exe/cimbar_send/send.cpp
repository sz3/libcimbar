/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_js/cimbar_js.h"

#include "cimb_translator/Config.h"
#include "serialize/str.h"
#include "util/File.h"

#include "cxxopts/cxxopts.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using std::string;
using std::vector;

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
	cxxopts::Options options("cimbar video encoder", "Draw a bunch of weird static on the screen!");

	unsigned colorBits = cimbar::Config::color_bits();
	unsigned compressionLevel = cimbar::Config::compression_level();
	unsigned ecc = cimbar::Config::ecc_bytes();
	unsigned defaultFps = 15;
	options.add_options()
		("i,in", "Source file", cxxopts::value<vector<string>>())
		("c,colorbits", "Color bits. [0-3]", cxxopts::value<int>()->default_value(turbo::str::str(colorBits)))
		("e,ecc", "ECC level", cxxopts::value<unsigned>()->default_value(turbo::str::str(ecc)))
		("f,fps", "Target FPS", cxxopts::value<unsigned>()->default_value(turbo::str::str(defaultFps)))
		("m,mode", "Select a cimbar mode. B is new to 0.6.x. 4C is the 0.5.x config. [B,Bm,Bu,4C]", cxxopts::value<string>()->default_value("4C"))
		("z,compression", "Compression level. 0 == no compression.", cxxopts::value<int>()->default_value(turbo::str::str(compressionLevel)))
		("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in"});
	options.positional_help("<in...>");

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  return 0;
	}

	vector<string> infiles = result["in"].as<vector<string>>();

	colorBits = std::min(3, result["colorbits"].as<int>());
	compressionLevel = result["compression"].as<int>();
	ecc = result["ecc"].as<unsigned>();

	unsigned config_mode = 68;
	if (result.count("mode"))
	{
		string mode = result["mode"].as<string>();
		if (mode == "4c" or mode == "4C")
			config_mode = 4;
		else if (mode == "Bu" or mode == "BU")
			config_mode = 66;
		else if (mode == "Bm" or mode == "BM")
			config_mode = 67;
	}

	unsigned fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = defaultFps;
	unsigned delay = 1000 / fps;

	int window_size_x = cimbar::Config::image_size_x() + 32;
	int window_size_y = cimbar::Config::image_size_y() + 32;
	if (cimbare_init_window(window_size_x, window_size_y) < 0)
	{
		std::cerr << "failed to create window :(" << std::endl;
		return 70;
	}
	cimbare_auto_scale_window();
	cimbare_configure(config_mode, compressionLevel);

	std::chrono::time_point start = std::chrono::high_resolution_clock::now();
	while (true)
		for (unsigned i = 0; i < infiles.size(); ++i)
		{
			// delay, then try to read file
			start = wait_for_frame_time(delay, start);
			// TODO: maybe delay is the wrong thing to do here. Might be best to just kick out any files that fail to read?
			// we can then error out properly if all inputs are bad, which would be nice.
			{
				const string& filename = infiles[i];
				string contents = File(filename).read_all();
				if (contents.empty())
				{
					std::cerr << "failed to read file " << filename << std::endl;
					continue;
				}

				if (cimbare_init_encode(filename.data(), filename.size(), -1) < 0)
				{
					std::cerr << "failed to 'init encode' file " << filename << std::endl;
					continue; // abort??
				}

				int res = cimbare_encode(reinterpret_cast<unsigned char*>(contents.data()), contents.size());
				if (res < 0)
				{
					std::cerr << "failed to compress file " << filename << std::endl;
					continue;
				}
				else if (res == 1 and cimbare_encode(nullptr, 0) != 0) // fallback finish encode
				{
					std::cerr << "failed to encode for file" << filename << std::endl;
					continue;
				}
			}

			// after loading our current file, render frames to the screen until next_frame() loops
			int frameCount = 0;
			do {
				start = wait_for_frame_time(delay, start);
				if (cimbare_render() < 0)
					return 0;
			}
			while (++frameCount == cimbare_next_frame()); // when next_frame() finally loops, we roll to the next file
		}

	return 0; // should never reach here
}
