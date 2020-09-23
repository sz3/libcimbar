/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Encoder.h"
#include "cimb_translator/Config.h"
#include "serialize/format.h"

#include <opencv2/opencv.hpp>
#include <functional>
#include <string>

class FileEncoder : public Encoder
{
public:
	using Encoder::Encoder;

	unsigned encode(const std::string& filename, std::string output_prefix);
	unsigned encode_fountain(const std::string& filename, std::string output_prefix, int compression_level=6);
	unsigned encode_fountain(const std::string& filename, const std::function<bool(const cv::Mat&, unsigned)>& on_frame, int compression_level=6);
};

inline unsigned FileEncoder::encode(const std::string& filename, std::string output_prefix)
{
	std::ifstream f(filename);

	unsigned i = 0;
	while (true)
	{
		auto frame = encode_next(f);
		if (!frame)
			break;

		std::string output = fmt::format("{}_{}.png", output_prefix, i);
		cv::imwrite(output, *frame);
		++i;
	}
	return i;
}

inline unsigned FileEncoder::encode_fountain(const std::string& filename, const std::function<bool(const cv::Mat&, unsigned)>& on_frame, int compression_level)
{
	std::ifstream infile(filename);
	fountain_encoder_stream::ptr fes = create_fountain_encoder(infile, compression_level);
	if (!fes)
		return 0;

	// With ecc = 40, we have 60 rs blocks * 115 bytes per block == 6900 bytes to work with.
	// the fountain_chunk_size will be 690.
	// fountain_chunks_per_frame() is currently a constant (10).
	unsigned requiredFrames = fes->blocks_required() * 2 / cimbar::Config::fountain_chunks_per_frame();
	if (requiredFrames == 0)
		requiredFrames = 1; // could also do +1 on the division above?

	unsigned i = 0;
	while (i < requiredFrames)
	{
		auto frame = encode_next(*fes);
		if (!frame)
			break;

		if (!on_frame(*frame, i))
			break;
		++i;
	}
	return i;
}

inline unsigned FileEncoder::encode_fountain(const std::string& filename, std::string output_prefix, int compression_level)
{
	std::function<bool(const cv::Mat&, unsigned)> fun = [output_prefix] (const cv::Mat& frame, unsigned i) {
		std::string output = fmt::format("{}_{}.png", output_prefix, i);
		return cv::imwrite(output, frame);
	};
	return encode_fountain(filename, fun, compression_level);
}
