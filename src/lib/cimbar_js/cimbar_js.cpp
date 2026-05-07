/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_js.h"

#include "cimb_translator/CimbWriter.h"
#include "cimb_translator/Config.h"
#include "compression/zstd_compressor.h"
#include "encoder/Encoder.h"
#include "gui/window_glfw.h"
#include "util/byte_istream.h"
#include <sstream>

namespace {
	std::shared_ptr<cimbar::window_glfw> _window;
	std::shared_ptr<fountain_encoder_stream> _fes;
	std::shared_ptr<CimbWriter> _gridWriter;

	// compressing the file
	std::unique_ptr<cimbar::zstd_compressor<std::stringstream>> _comp;

	int _frameCount = 0;
	// start encode_id is 109. This is mostly unimportant (it only needs to wrap between [0,127]), but useful
	// for the decoder -- because it gives it a better distribution of colors in the first frame header it sees.
	uint8_t _encodeId = 109;

	// settings, will be overriden by first call to configure()
	int _modeVal = 68;
	int _compressionLevel = cimbar::Config::compression_level();
}

extern "C" {

int cimbare_init_window(int width, int height)
{
	if (width <= 0)
		width = cimbar::Config::image_size_x() + 16;
	if (height <= 0)
		height = cimbar::Config::image_size_y() + 16;

	// must be divisible by 4???
	if (width % 4 != 0)
		width += (4 - width % 4);
	if (height % 4 != 0)
		height += (4 - height % 4);
	std::cerr << "initializing " << width << " by " << height << " window";

	if (_window and _window->is_good())
		_window->resize(width, height);
	else
		_window = std::make_shared<cimbar::window_glfw>(width, height, "Cimbar Encoder");
	if (!_window or !_window->is_good())
		return -1;

	return 0;
}

int cimbare_rotate_window(bool rotate)
{
	if (!_window or !_window->is_good())
		return -1;

	_window->rotate(0);
	if (rotate) // 90 degrees
	{
		_window->rotate();
		_window->rotate();
	}
	return 0;
}

bool cimbare_auto_scale_window(unsigned padding)
{
	if (!_window or !_window->is_good())
		return false;

	_window->auto_scale_to_window(padding);
	return true;
}

// we may change the api to accept an buff to next_frame()
// rather than generating a fresh cv::Mat alloc each time
// but for now, for non-JS purposes we expose this function
int cimbare_get_frame_buff(unsigned char* buff, unsigned size)
{
	return -1;
}

// render() and next_frame() could be put in the same function,
// but it seems cleaner to split them.
// in any case, we're concerned with frame pacing (some encodes take longer than others)
int cimbare_render()
{
	if (!_window or !_fes or _window->should_close())
		return -1;

	if (_gridWriter)
	{
		_window->show(_gridWriter->image(), 0);
		_window->shake();
		return 1;
	}
	return 0;
}

int cimbare_next_frame()
{
	if (!_fes)
		return -1;

	// we generate 8x the amount of required symbol blocks.
	// this number is somewhat arbitrary, but needs to not be
	// *too* low (1-2), or we risk long runs of blocks the decoder
	// has already seen.
	unsigned required = _fes->blocks_required() * 8;
	if (_fes->block_count() > required)
	{
		_fes->restart();
		if (_window)
			_window->shake(0);
		_frameCount = 0;
	}

	Encoder enc;
	enc.set_encode_id(_encodeId);

	// let's move CimbWriter out so its tiles are cached. How much will that help perf??? :thinking:
	// prob not as much as GPU, but it's a necessary step anyway...
	enc.encode_next(*_fes, *_gridWriter);
	return ++_frameCount;
}

// maybe init_encode w/ filename,size,encode_id,
// then encode() with buff,size? ... when size < chunksize (or size ==0), we're done
// return 0 on done, 1 iff work to continue?

int cimbare_init_encode(const char* filename, unsigned fnsize, int encode_id)
{
	_frameCount = 0;
	if (!FountainInit::init())
	{
		std::cerr << "failed FountainInit :(" << std::endl;
		return -5;
	}

	if (encode_id < 0)
		++_encodeId; // increment _encodeId every time we change files
	else
		_encodeId = encode_id;

	_comp = std::make_unique<cimbar::zstd_compressor<std::stringstream>>();
	if (!_comp)
		return -1;

	_comp->set_compression_level(_compressionLevel);

	if (fnsize > 0 and filename != nullptr)
		_comp->write_header(filename, fnsize);

	_fes.reset();
	return 0;
}

int cimbare_encode_bufsize()
{
	return cimbar::zstd_compressor<std::stringstream>::CHUNK_SIZE;
}

int cimbare_encode(const unsigned char* buffer, unsigned size)
{
	if (!_comp)
		return -1;

	if (size > 0)
	{
		if (!_comp->write(reinterpret_cast<const char*>(buffer), size))
			return -2;
	}
	if (size == cimbare_encode_bufsize())
		return 1; // more to do

	// otherwise, we're ready
	unsigned fountainChunkSize = cimbar::Config::fountain_chunk_size();
	size_t compressedSize = _comp->size();
	if (compressedSize < fountainChunkSize)
		_comp->pad(fountainChunkSize - compressedSize + 1);

	// prepare writer
	// TODO: prob use Encoder helpers for both these constructors?
	_gridWriter = std::make_shared<CimbWriter>(cimbar::Config::symbol_bits(), cimbar::Config::color_bits());

	// create the encoder stream
	_fes = fountain_encoder_stream::create(*_comp, fountainChunkSize, _encodeId);
	_comp.reset();
	if (!_fes)
		return -3;

	return 0;
}

int cimbare_configure(int mode_val, int compression, bool color_balance)
{
	cimbar::Config::update(mode_val);
	if (compression < 0 or compression > 22)
		compression = cimbar::Config::compression_level();

	// check if we need to refresh the stream
	bool refresh = (mode_val != _modeVal or compression != _compressionLevel);
	if (refresh)
	{
		// update config
		_modeVal = mode_val;
		_compressionLevel = compression;
		cimbar::Config::update(_modeVal);

		// reset writer
		// if window,
		_gridWriter = std::make_shared<CimbWriter>(
			cimbar::Config::symbol_bits(), cimbar::Config::color_bits(),
			true, cimbar::Config::color_mode() + color_balance? 0x100 : 0,
			_window? cimbar::vec_xy{_window->width(), _window->height()} : cimbar::vec_xy{}
		);

		// try to refresh the stream
		if (_fes)
		{
			unsigned buff_size_new = cimbar::Config::fountain_chunk_size();
			if (!_fes->restart_and_resize_buffer(buff_size_new))
			{
				// if the data is too small, we should throw out _fes -- and clear the canvas.
				_fes = nullptr;
				if (_window)
					_window->clear();
			}
			_frameCount = 0;
			if (_window)
				_window->shake(0); //reset
		}
	}
	return 0;
}

float cimbare_get_aspect_ratio()
{
	// based on the current config
	// we use +16 to match configure()
	float window_size_x = cimbar::Config::image_size_x() + 16;
	float window_size_y = cimbar::Config::image_size_y() + 16;
	return window_size_x / window_size_y;
}


}
