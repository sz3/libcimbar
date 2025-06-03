/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_recv_js.h"

#include "cimb_translator/Config.h"
#include "encoder/Decoder.h"
#include "encoder/escrow_buffer_writer.h"
#include "extractor/Extractor.h"
#include "fountain/fountain_decoder_sink.h"
#include "serialize/str_join.h"

#include <opencv2/opencv.hpp>

#include <memory>


namespace {

	std::shared_ptr<fountain_decoder_sink> _sink;

	// settings
	unsigned _colorBits = 2; // call configure() for defaults
	int _modeVal = 68;

	bool legacy_mode()
	{
		return _modeVal == 4;
	}
}

extern "C" {

int do_decode(uchar* rgba_image_data, int width, int height)
{
	if (!rgba_image_data)
		return -1;

	int totalRed = 0;
	int count = 0;

	int stride = 4;
	int len = width*height*stride;
	for (int i = 0; i < len; i+=stride)
	{
		totalRed += rgba_image_data[i];
		++count;
	}
	if (!count)
		++count;
	return totalRed/count;
}

int fountain_get_bufsize()
{
	unsigned chunksPerFrame = cimbar::Config::fountain_chunks_per_frame(cimbar::Config::symbol_bits() + _colorBits, legacy_mode());
	unsigned chunkSize = cimbar::Config::fountain_chunk_size(
				cimbar::Config::ecc_bytes(),
				cimbar::Config::symbol_bits() + _colorBits,
				legacy_mode()
	);
	return chunksPerFrame * chunkSize;
}

int scan_extract_decode(uchar* imgdata, unsigned imgw, unsigned imgh, uchar* bufspace, unsigned bufsize)
{
	if (imgw == 0 or imgh == 0)
		return -1;

	unsigned chunksPerFrame = cimbar::Config::fountain_chunks_per_frame(cimbar::Config::symbol_bits() + _colorBits, legacy_mode());
	unsigned chunkSize = cimbar::Config::fountain_chunk_size(
				cimbar::Config::ecc_bytes(),
				cimbar::Config::symbol_bits() + _colorBits,
				legacy_mode()
	);
	// early bail if bufsize doesn't match config params (fountain chunk size * count)
	if (bufsize != chunkSize * chunksPerFrame)
		return -2;

	// need a class that implements the writer/sink interface and writes to our "escrow" buffers
	escrow_buffer_writer ebw(bufspace, chunksPerFrame, chunkSize);

	// at the end, return abw.num_writes()

	Extractor ext;
	Decoder dec(-1, -1);

	cv::UMat img = cv::Mat(imgh, imgw, CV_8UC3, (void*)imgdata).getUMat(cv::ACCESS_RW).clone();

	bool shouldPreprocess = true;
	int res = ext.extract(img, img);
	if (!res)
		return -3;
	else if (res == Extractor::NEEDS_SHARPEN)
		shouldPreprocess = true;

	// decode
	int bytes = dec.decode_fountain(img, ebw, legacy_mode()? 0 : 1, shouldPreprocess);
	return ebw.buffers_in_use() * chunkSize;
}

// returns id of final file (can be used to get size of `finish_copy`'s buffer) if complete, 0 if success, -1 on error
int64_t fountain_decode(unsigned char* buffer, unsigned size)
{
	if (!_sink)
	{
		// lazy-create the sink on first run
		unsigned chunkSize = cimbar::Config::fountain_chunk_size(
					cimbar::Config::ecc_bytes(),
					cimbar::Config::symbol_bits() + _colorBits,
					legacy_mode()
		);
		_sink = std::make_shared<fountain_decoder_sink>(chunkSize);
	}

	unsigned chunkSize = cimbar::Config::fountain_chunk_size(
				cimbar::Config::ecc_bytes(),
				cimbar::Config::symbol_bits() + _colorBits,
				legacy_mode()
	);
	if (size == 0 or size % chunkSize != 0)
		return -2;

	uint32_t res = 0;
	for (unsigned i = 0; i < size && res == 0; i+=chunkSize)
		res = _sink->decode_frame(reinterpret_cast<char*>(buffer+i), chunkSize);

	// res will be the file id on completion, 0 otherwise
	std::cout << "progress: " << turbo::str::join(_sink->get_progress()) << std::endl;
	return res;
}

int fountain_get_filesize(uint32_t id)
{
	FountainMetadata md(id);
	return md.file_size();
}

int fountain_get_filename(uint32_t id, char* buf, unsigned size)
{
	if (!_sink)
		return -1;

	FountainMetadata md(id);
	std::string filename = _sink->get_filename(md);
	if (filename.size() > size)
		filename.resize(size);
	std::copy(filename.begin(), filename.end(), buf);
	return filename.size();
}

// if fountain_decode returned a >0 value, call this to retrieve the reassembled file
// bouth fountain_*() calls should be from the same js webworker/thread
int fountain_finish_copy(uint32_t id, uchar* buffer, unsigned size)
{
	if (!_sink)
		return -1;
	if (!_sink->recover(id, buffer, size))
		return -2;
	return 0;
}

int configure_decode(unsigned color_bits, int mode_val)
{
	// defaults
	if (color_bits > 3)
		color_bits = cimbar::Config::color_bits();
	if (mode_val <= 0)
		mode_val = 68;

	bool refresh = (color_bits != _colorBits or mode_val != _modeVal);
	if (refresh)
	{
		// update config
		_colorBits = color_bits;
		_modeVal = mode_val;
		_sink.reset();
	}

	return 0;
}

}
