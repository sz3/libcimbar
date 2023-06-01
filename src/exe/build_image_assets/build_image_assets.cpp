/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "util/File.h"

#include "base91/base.hpp"
#include "cxxopts/cxxopts.hpp"
#include "serialize/format.h"

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;

map<string, string> getFileBlobs(string dir_path)
{
	// std::filesystem is still hard to get the compiler to use, so we'll manually enumerate for now
	map<string, string> blobs;

	vector<string> anchors = {"anchor-{}.png", "anchor-secondary-{}.png", "guide-horizontal-{}.png", "guide-vertical-{}.png"};
	for (const string& mode : {"light", "dark"})
		for (const string& a : anchors)
		{
			string short_path = fmt::format(a, mode);
			string file_path = fmt::format("{}/{}", dir_path, short_path);
			string contents = File(file_path).read_all();
			blobs[file_path] = base91::encode(contents);
		}

	for (int i = 0; i < 4; ++i)
	{
		string file_path = fmt::format("{}/2/{:02x}.png", dir_path, i);
		string contents = File(file_path).read_all();
		blobs[file_path] = base91::encode(contents);
	}
	for (int i = 0; i < 16; ++i)
	{
		string file_path = fmt::format("{}/4/{:02x}.png", dir_path, i);
		string contents = File(file_path).read_all();
		blobs[file_path] = base91::encode(contents);
	}
	return blobs;
}

int main(int argc, char** argv)
{
	cxxopts::Options options("build_image_assets", "Build a C++ class file (header-only) that contains base91 png asset data.");

	options.add_options()
		("b,bitmap", "Bitmap directory", cxxopts::value<std::string>())
		("h,help", "Print usage")
	;

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("bitmap"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	std::string bitmapDir = result["bitmap"].as<std::string>();
	std::cout << "got bitmapDir, it's " << bitmapDir << std::endl;

	std::ofstream out("bitmaps.h");
	out << "namespace cimbar {" << std::endl;
	out << "static const std::map<std::string, std::string> bitmaps = {" << std::endl;

	map<string, string> blobs = getFileBlobs(bitmapDir);
	for (auto const& [key, val] : blobs)
		out << "{\"" << key << "\", R\"(" << val << ")\"}," << std::endl;

	out << "};" << std::endl;
	out << "}" << std::endl;
	return 0;
}
