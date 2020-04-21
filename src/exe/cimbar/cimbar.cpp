
#include "encoder/Decoder.h"
#include "extractor/Extractor.h"

#include "cxxopts/cxxopts.hpp"

#include <string>
using std::string;

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar decoder", "Test decode program! Probably not as user friendly as the python version!");

	options.add_options()
	    ("i,in", "Encoded png/jpg/etc", cxxopts::value<std::string>())
	    ("o,out", "Output file", cxxopts::value<std::string>())
	    ("e,ecc", "ECC level (default: 10)", cxxopts::value<unsigned>())
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
	unsigned ecc = 10;
	if (result.count("ecc"))
		ecc = result["ecc"].as<unsigned>();

	Extractor ext;
	cv::Mat img;
	if (!ext.extract(infile, img))
		return 1;

	Decoder d(ecc);
	unsigned bytes = d.decode(img, outfile);
	if (bytes == 0)
		return 2;
	return 0;
}
