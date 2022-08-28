/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "extractor/Extractor.h"

#include "cxxopts/cxxopts.hpp"

#include <string>
using std::string;

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar_extract", "Scan/extract cimbar code from a source image.");

	options.add_options()
	    ("i,in", "Encoded png/jpg/etc", cxxopts::value<std::string>())
	    ("o,out", "Output image", cxxopts::value<std::string>())
	    ("d,dark", "Dark mode", cxxopts::value<bool>()->default_value("true"))
	    ("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in", "out"});
	options.positional_help("<in> <out>");

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("out") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	std::string infile = result["in"].as<std::string>();
	std::string outfile = result["out"].as<std::string>();
	bool dark = result["dark"].as<bool>();

	Extractor ext;
	if (!ext.extract(infile, outfile))
		return 1;

	return 0;
}
