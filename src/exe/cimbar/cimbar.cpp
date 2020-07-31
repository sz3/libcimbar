
#include "cimb_translator/Config.h"
#include "encoder/Decoder.h"
#include "encoder/Encoder.h"
#include "extractor/Extractor.h"
#include "extractor/SimpleCameraCalibration.h"
#include "extractor/Undistort.h"
#include "fountain/FountainInit.h"
#include "fountain/fountain_decoder_sink.h"

#include "cxxopts/cxxopts.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

int decode(const vector<string>& infiles, std::function<int(cv::UMat, bool)>& decode, bool no_deskew, bool undistort, int preprocess)
{
	int err = 0;
	Undistort<SimpleCameraCalibration> und;
	for (const string& inf : infiles)
	{
		bool shouldPreprocess = (preprocess == 1);
		cv::UMat img = cv::imread(inf).getUMat(cv::ACCESS_RW);
		if (!no_deskew)
		{
			// attempt undistort
			// we don't fail outright, but we'll probably fail the decode :(
			if (undistort)
			{
				if (!und.undistort(img, img))
					err |= 1;
			}

			Extractor ext;
			int res = ext.extract(img, img);
			if (!res)
			{
				err |= 2;
				continue;
			}
			else if (preprocess != 0 and res == Extractor::NEEDS_SHARPEN)
				shouldPreprocess = true;
		}

		int bytes = decode(img, shouldPreprocess);
		if (!bytes)
			err |= 4;
	}
	return err;
}

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar decoder", "Test decode program! Probably not as user friendly as the python version!");

	options.add_options()
	    ("i,in", "Encoded png/jpg/etc", cxxopts::value<vector<string>>())
	    ("o,out", "Output file or directory", cxxopts::value<string>())
	    ("e,ecc", "ECC level (default: 40)", cxxopts::value<unsigned>())
	    ("f,fountain", "Attempt fountain decoding", cxxopts::value<bool>())
	    ("encode", "Run the encoder!", cxxopts::value<bool>())
	    ("no-deskew", "Skip the deskew step -- treat input image as already extracted.", cxxopts::value<bool>())
	    ("undistort", "Attempt undistort step -- useful if image distortion is significant.", cxxopts::value<bool>())
	    ("preprocess", "Force the sharpen/preprocessing filter to run on the input image. 1 == on. 0 == off. Default is to guess.", cxxopts::value<int>())
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
	unsigned ecc = cimbar::Config::ecc_bytes();
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
	bool undistort = result.count("undistort");
	int preprocess = -1;
	if (result.count("preprocess"))
		preprocess = result["preprocess"].as<int>();

	Decoder d(ecc);

	if (fountain)
	{
		fountain_decoder_sink sink(outpath, cimbar::Config::fountain_chunk_size(ecc));
		std::function<int(cv::UMat,bool)> fun = [&sink, &d] (cv::UMat m, bool pre) {
			return d.decode_fountain(m, sink, pre);
		};
		return decode(infiles, fun, no_deskew, undistort, preprocess);
	}

	// else
	std::ofstream f(outpath);
	std::function<int(cv::UMat,bool)> fun = [&f, &d] (cv::UMat m, bool pre) {
		return d.decode(m, f, pre);
	};
	return decode(infiles, fun, no_deskew, undistort, preprocess);
}
