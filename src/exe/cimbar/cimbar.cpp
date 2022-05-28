/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimb_translator/Config.h"
#include "compression/zstd_decompressor.h"
#include "encoder/Decoder.h"
#include "encoder/Encoder.h"
#include "extractor/Extractor.h"
#include "extractor/SimpleCameraCalibration.h"
#include "extractor/Undistort.h"
#include "fountain/FountainInit.h"
#include "fountain/fountain_decoder_sink.h"
#include "serialize/str.h"

#include "cxxopts/cxxopts.hpp"

#include <experimental/filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace {
	class stdinerator
	{
	public:
		stdinerator(bool done=false)
			: _done(done)
		{
			read_once();
		}

		void mark_done()
		{
			_done = true;
		}

		void read_once()
		{
			if (!_done)
				std::getline(std::cin, _current);
		}

		std::string operator*() const
		{
			return _current;
		}

		stdinerator& operator++()
		{
			read_once();
			return *this;
		}

		bool operator==(const stdinerator& rhs) const
		{
			return _done and rhs._done;
		}

		bool operator!=(const stdinerator& rhs) const
		{
			return !operator==(rhs);
		}

		static stdinerator begin()
		{
			return stdinerator();
		}

		static stdinerator end()
		{
			return stdinerator(true);
		}

	protected:
		bool _done = false;
		std::string _current;
	};

	struct StdinLineReader
	{
		stdinerator begin() const
		{
			return stdinerator::begin();
		}

		stdinerator end() const
		{
			return stdinerator::end();
		}
	};
}

template <typename FileIterable>
int decode(const FileIterable& infiles, const std::function<int(cv::UMat, bool, bool)>& decode, bool no_deskew, bool undistort, int preprocess, bool color_correct)
{
	int err = 0;
	for (const string& inf : infiles)
	{
		bool shouldPreprocess = (preprocess == 1);
		cv::UMat img = cv::imread(inf).getUMat(cv::ACCESS_RW);
		cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

		if (!no_deskew)
		{
			// attempt undistort. It's currently a low-effort attempt to *reduce* distortion, not eliminate it.
			// we rely on the decoder to power through minor distortion
			if (undistort)
			{
				Undistort<SimpleCameraCalibration> und;
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

		int bytes = decode(img, shouldPreprocess, color_correct);
		if (!bytes)
			err |= 4;
	}
	return err;
}

template <typename SINK>
std::function<int(cv::UMat,bool,bool)> fountain_decode_fun(SINK& sink, Decoder& d)
{
	return [&sink, &d] (cv::UMat m, bool pre, bool cc) {
		return d.decode_fountain(m, sink, pre, cc);
	};
}

int main(int argc, char** argv)
{
	cxxopts::Options options("cimbar encoder/decoder", "Demonstration program for cimbar codes");

	unsigned colorBits = cimbar::Config::color_bits();
	unsigned compressionLevel = cimbar::Config::compression_level();
	unsigned ecc = cimbar::Config::ecc_bytes();
	options.add_options()
		("i,in", "Encoded pngs/jpgs/etc (for decode), or file to encode", cxxopts::value<vector<string>>())
		("o,out", "Output file prefix (encoding) or directory (decoding).", cxxopts::value<string>())
		("c,color-bits", "Color bits. [0-3]", cxxopts::value<int>()->default_value(turbo::str::str(colorBits)))
		("e,ecc", "ECC level", cxxopts::value<unsigned>()->default_value(turbo::str::str(ecc)))
		("z,compression", "Compression level. 0 == no compression.", cxxopts::value<int>()->default_value(turbo::str::str(compressionLevel)))
		("color-correct", "Toggle decoding color correction. 1 == on. 0 == off.", cxxopts::value<int>()->default_value("1"))
		("encode", "Run the encoder!", cxxopts::value<bool>())
		("no-deskew", "Skip the deskew step -- treat input image as already extracted.", cxxopts::value<bool>())
		("no-fountain", "Disable fountain encode/decode. Will also disable compression.", cxxopts::value<bool>())
		("undistort", "Attempt undistort step -- useful if image distortion is significant.", cxxopts::value<bool>())
		("preprocess", "Run sharpen filter on the input image. 1 == on. 0 == off. -1 == guess.", cxxopts::value<int>()->default_value("-1"))
		("h,help", "Print usage")
	;
	options.show_positional_help();
	options.parse_positional({"in"});
	options.positional_help("<in...>");

	auto result = options.parse(argc, argv);
	if (result.count("help"))
	{
		std::cerr << options.help() << std::endl;
		exit(0);
	}

	string outpath = std::experimental::filesystem::current_path();
	if (result.count("out"))
		outpath = result["out"].as<string>();
	std::cerr << "Output files will appear in " << outpath << std::endl;

	bool useStdin = !result.count("in");
	vector<string> infiles;
	if (useStdin)
		std::cerr << "Enter input filenames:" << std::endl;
	else
		infiles = result["in"].as<vector<string>>();

	bool encode = result.count("encode");
	bool no_fountain = result.count("no-fountain");

	colorBits = std::min(3, result["color-bits"].as<int>());
	compressionLevel = result["compression"].as<int>();
	ecc = result["ecc"].as<unsigned>();

	if (encode)
	{
		Encoder en(ecc, cimbar::Config::symbol_bits(), colorBits);
		for (const string& f : infiles)
		{
			if (no_fountain)
				en.encode(f, outpath);
			else
				en.encode_fountain(f, outpath, compressionLevel);
		}
		return 0;
	}

	// else, decode
	bool no_deskew = result.count("no-deskew");
	bool undistort = result.count("undistort");
	int color_correct = result["color-correct"].as<int>();
	int preprocess = result["preprocess"].as<int>();

	Decoder d(ecc, colorBits);

	if (no_fountain)
	{
		// simpler encoding, just the basics + ECC. No compression, fountain codes, etc.
		std::ofstream f(outpath);
		std::function<int(cv::UMat,bool,bool)> fun = [&f, &d] (cv::UMat m, bool pre, bool cc) {
			return d.decode(m, f, pre, cc);
		};
		if (useStdin)
			return decode(StdinLineReader(), fun, no_deskew, undistort, preprocess, color_correct);
		else
			return decode(infiles, fun, no_deskew, undistort, preprocess, color_correct);
	}

	// else, the good stuff
	unsigned chunkSize = cimbar::Config::fountain_chunk_size(ecc);
	if (compressionLevel <= 0)
	{
		fountain_decoder_sink<std::ofstream> sink(outpath, chunkSize);
		return decode(infiles, fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
	}

	// else -- default case, all bells and whistles
	fountain_decoder_sink<cimbar::zstd_decompressor<std::ofstream>> sink(outpath, chunkSize);

	if (useStdin)
		return decode(StdinLineReader(), fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
	else
		return decode(infiles, fountain_decode_fun(sink, d), no_deskew, undistort, preprocess, color_correct);
}
