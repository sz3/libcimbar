
#include "extractor/Extractor.h"

#include "cxxopts/cxxopts.hpp"

#include <string>
using std::string;

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar scan", "Scan/extract cimbar code from a source image.");

	options.add_options()
		("i,in", "Encoded png/jpg/etc", cxxopts::value<std::string>())
		("o,out", "Output image", cxxopts::value<std::string>())
		("d,dark", "Dark mode (default: true)", cxxopts::value<bool>())
		("h,help", "Print usage")
	;

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("out") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	std::string infile = result["in"].as<std::string>();
	std::string outfile = result["out"].as<std::string>();
	bool dark = true;
	if (result.count("dark"))
		dark = result["dark"].as<bool>();

	Extractor ext;
	if (!ext.extract(infile, outfile))
		return 1;

	return 0;
}
