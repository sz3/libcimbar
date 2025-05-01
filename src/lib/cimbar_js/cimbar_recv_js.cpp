/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_recv_js.h"

#include "cimb_translator/Config.h"
#include "encoder/Decoder.h"
#include "encoder/escrow_buffer_writer.h"
#include "extractor/Extractor.h"
#include "fountain/fountain_decoder_sink.h"

#include <opencv2/opencv.hpp>

#include <memory>


namespace {

	std::shared_ptr<fountain_decoder_sink> _sink;


	// settings
	unsigned _colorBits = 255; // call configure() for defaults
	int _modeVal = 0;

}

extern "C" {


int scan_extract_decode(char* imgdata, unsigned imgw, unsigned imgh, char** buffers, unsigned bufcount, unsigned bufsize)
{
	// TODO: early bail if bufsize doesn't match config params (fountain chunk size)
	// if bad imgsize
	// return -2
	// if bad bufsize
	// return -3;

	// need a class that implements the writer/sink interface and writes to our "escrow" buffers
	escrow_buffer_writer ebw(buffers, bufcount, bufsize);

	// at the end, return abw.num_writes()

	Extractor ext;
	Decoder dec(-1, -1);

	cv::Mat img(imgw, imgh, CV_8UC3, (void*)imgdata);
	cv::Mat extracted;

	bool shouldPreprocess = true;
	int res = ext.extract(img, extracted);
	if (!res)
		return -4;
	else if (res == Extractor::NEEDS_SHARPEN)
		shouldPreprocess = true;

	// decode
	int bytes = dec.decode_fountain(img, ebw, _modeVal==4? 0 : 1, shouldPreprocess);
	return ebw.buffers_in_use();
}

// returns size of final file (size of `finish_copy`'s buffer) if complete, 0 if success, -1 if failure
int fountain_decode(char* buffer, unsigned size)
{
	if (!_sink)
		return -1;

	_sink->write(buffer, size);
	// _sink.write(); // if true, return size of file
	return 0;
}

// if fountain_decode returned a >0 value, call this to retrieve the reassembled file
// bouth fountain_*() calls should be from the same js webworker/thread
int fountain_finish_copy(char* buffer, unsigned size)
{
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
		unsigned chunkSize = cimbar::Config::fountain_chunk_size(
					cimbar::Config::ecc_bytes(),
					cimbar::Config::symbol_bits() + color_bits,
					mode_val==4);

		// TODO: gotta give the final write a place to go
		_sink = std::make_shared<fountain_decoder_sink>(chunkSize);
	}

	return 0;
}

}
