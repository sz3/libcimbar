
#include "encoder/Decoder.h"
#include "encoder/Encoder.h"
#include "extractor/Extractor.h"
#include "fountain/FountainInit.h"
#include "fountain/fountain_decoder_sink.h"

#include "cxxopts/cxxopts.hpp"

#include <functional>
#include <string>
#include <vector>
using std::string;
using std::vector;

int decode(const vector<string>& infiles, std::function<int(cv::Mat, bool)>& decode, bool no_deskew, bool force_preprocess)
{
	for (const string& inf : infiles)
	{
		bool shouldPreprocess = force_preprocess;
		cv::Mat img;
		if (no_deskew)
			img = cv::imread(inf);
		else
		{
			Extractor ext;
			int res = ext.extract(inf, img);
			if (!res)
				return 1;
			else if (res == Extractor::NEEDS_SHARPEN)
				shouldPreprocess = true;
		}

		int bytes = decode(img, shouldPreprocess);
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
	    ("e,ecc", "ECC level (default: 30)", cxxopts::value<unsigned>())
	    ("f,fountain", "Attempt fountain decoding", cxxopts::value<bool>())
	    ("encode", "Run the encoder!", cxxopts::value<bool>())
	    ("no-deskew", "Skip the deskew step -- treat input image as already extracted.", cxxopts::value<bool>())
	    ("force-preprocess", "Force the sharpen/preprocessing filter to run on the input image.", cxxopts::value<bool>())
	    ("h,help", "Print usage")
	;

	auto result = options.parse(argc, argv);
	if (result.count("help") or !result.count("out") or !result.count("in"))
	{
	  std::cout << options.help() << std::endl;
	  exit(0);
	}

	vector<string> infiles = result["in"].as<vector<string>>();
	string outpath = result["out"].as<string>();

	bool encode = result.count("encode");
	bool fountain = result.count("fountain");
	unsigned ecc = 30;
	if (result.count("ecc"))
		ecc = result["ecc"].as<unsigned>();

	if (fountain)
		FountainInit::init();

	if (encode)
	{
		Encoder en(ecc);
		for (const string& f : infiles)
		{
			if (fountain)
				en.encode_fountain(f, outpath);
			else
				en.encode(f, outpath);
		}
		return 0;
	}

	// else, decode
	bool no_deskew = result.count("no-deskew");
	bool force_preprocess = result.count("force-preprocess");
	Decoder d(ecc);

	if (fountain)
	{
		fountain_decoder_sink<394> sink(outpath);
		std::function<int(cv::Mat,bool)> fun = [&sink, &d] (cv::Mat m, bool pre) {
			return d.decode_fountain(m, sink, pre);
		};
		return decode(infiles, fun, no_deskew, force_preprocess);
	}

	// else
	std::ofstream f(outpath);
	std::function<int(cv::Mat,bool)> fun = [&f, &d] (cv::Mat m, bool pre) {
		return d.decode(m, f, pre);
	};
	return decode(infiles, fun, no_deskew, force_preprocess);
}
