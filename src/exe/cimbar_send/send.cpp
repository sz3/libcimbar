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
	unsigned fps = result["fps"].as<unsigned>();
	if (fps == 0)
		fps = defaultFps;
	unsigned delay = 1000 / fps;

	int window_size = cimbar::Config::image_size() + 32;
	if (!initialize_GL(window_size, window_size))
	{
		std::cerr << "failed to create window :(" << std::endl;
		return 70;
	}

	configure(colorBits, ecc, compressionLevel);

	std::chrono::time_point start = std::chrono::high_resolution_clock::now();
	while (true)
		for (unsigned i = 0; i < infiles.size(); ++i)
		{
			// delay, then try to read file
			start = wait_for_frame_time(delay, start);
			// TODO: maybe delay is the wrong thing to do here. Might be best to just kick out any files that fail to read?
			// we can then error out properly if all inputs are bad, which would be nice.
			{
				string contents = File(infiles[i]).read_all();
				if (contents.empty())
				{
					std::cerr << "failed to read file " << infiles[i] << std::endl;
					continue;
				}

				if (!encode(reinterpret_cast<unsigned char*>(contents.data()), contents.size(), static_cast<int>(i)))
				{
					std::cerr << "failed to encode file " << infiles[i] << std::endl;
					continue;
				}
			}

			// after loading our current file, render frames to the screen until next_frame() loops
			int frameCount = 0;
			do {
				start = wait_for_frame_time(delay, start);
				if (render() < 0)
					return 0;
			}
			while (++frameCount == next_frame()); // when next_frame() finally loops, we roll to the next file
		}

	return 0; // should never reach here
}
