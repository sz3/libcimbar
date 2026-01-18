/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimb_translator/Config.h"
#include "extractor/ExtractorPlus.h"

#include "cxxopts/cxxopts.hpp"

#include <string>
using std::string;

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar_extract", "Scan/extract cimbar code from a source image.");

	options.add_options()
	    ("i,in", "Encoded png/jpg/etc", cxxopts::value<std::string>())
	    ("o,out", "Output image", cxxopts::value<std::string>())
	    ("m,mode", "Select a cimbar mode. B (the default) is new to 0.6.x. 4C is the 0.5.x config. [B,Bm,Bu,4C]", cxxopts::value<string>()->default_value("B"))
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

	// really, you only need to supply mode for non 1024x1024 configs
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
	cimbar::Config::update(config_mode);

	std::string infile = result["in"].as<std::string>();
	std::string outfile = result["out"].as<std::string>();
	ExtractorPlus ext;
	if (!ext.extract(infile, outfile))
		return 1;

	return 0;
}
