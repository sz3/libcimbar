
#include "encoder/Decoder.h"
#include "extractor/Extractor.h"
#include "fountain/FountainInit.h"
#include "fountain/fountain_decoder_sink.h"

#include "cxxopts/cxxopts.hpp"

#include <string>
#include <vector>
using std::string;
using std::vector;

template <typename STREAM>
int decode(const vector<string>& infiles, STREAM& ostream, Decoder& d, bool no_deskew)
{
	for (const string& inf : infiles)
	{
		cv::Mat img;
		if (no_deskew)
			img = cv::imread(inf);
		else
		{
			Extractor ext;
			if (!ext.extract(inf, img))
				return 1;
		}

		unsigned bytes = d.decode(img, ostream);
		if (bytes == 0)
			return 2;
	}
	return 0;
}

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar decoder", "Test decode program! Probably not as user friendly as the python version!");

	options.add_options()
	    ("i,in", "Encoded png/jpg/etc", cxxopts::value<vector<string>>())
	    ("o,out", "Output file or directory", cxxopts::value<string>())
	    ("e,ecc", "ECC level (default: 15)", cxxopts::value<unsigned>())
	    ("f,fountain", "Attempt fountain decoding", cxxopts::value<bool>())
	    ("no-deskew", "Skip the deskew step -- treat input image as pre-processed.", cxxopts::value<bool>())
	    ("h,help", "Print usage")
	;

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("out") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	vector<string> infiles = result["in"].as<vector<string>>();
	string outfile = result["out"].as<string>();

	bool no_deskew = result.count("no-deskew");
	bool fountain = result.count("fountain");
	unsigned ecc = 15;
	if (result.count("ecc"))
		ecc = result["ecc"].as<unsigned>();
	Decoder d(ecc);

	if (fountain)
	{
		FountainInit::init();
		fountain_decoder_sink<599> sink(outfile);
		return decode(infiles, sink, d, no_deskew);
	}

	// else
	std::ofstream f(outfile);
	return decode(infiles, f, d, no_deskew);
}
