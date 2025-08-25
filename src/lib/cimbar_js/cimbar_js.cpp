/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_js.h"

#include "cimb_translator/Config.h"
#include "encoder/Encoder.h"
#include "gui/window_glfw.h"
#include "util/byte_istream.h"

#include <iostream>
#include <sstream>


namespace {
	std::shared_ptr<cimbar::window_glfw> _window;
	std::shared_ptr<fountain_encoder_stream> _fes;
	std::optional<cv::Mat> _next;

	int _frameCount = 0;
	uint8_t _encodeId = 109;

	// settings, will be overriden by first call to configure()
	int _modeVal = 68;
	int _compressionLevel = cimbar::Config::compression_level();
}

extern "C" {

int cimbare_init_window(int width, int height)
{
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

bool cimbare_auto_scale_window()
{
	if (!_window or !_window->is_good())
		return false;

	_window->auto_scale_to_window();
	return true;
}

// we may change the api to accept an buff to next_frame()
// rather than generating a fresh cv::Mat alloc each time
// but for now, for non-JS purposes we expose this function
int cimbare_get_frame_buff(unsigned char** buff)
{
	if (!_next)
		return -2;
	if (_next->cols == 0 or _next->rows == 0)
		return -1;

	*buff = _next->data;
	return _next->cols * _next->rows * _next->channels();
}

// render() and next_frame() could be put in the same function,
// but it seems cleaner to split them.
// in any case, we're concerned with frame pacing (some encodes take longer than others)
int cimbare_render()
{
	if (!_window or !_fes or _window->should_close())
		return -1;

	if (_next)
	{
		_window->show(*_next, 0);
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
	_next = enc.encode_next(*_fes, _window? cimbar::vec_xy{_window->width(), _window->height()} : cimbar::vec_xy{});
	return ++_frameCount;
}

int cimbare_encode(const unsigned char* buffer, unsigned size, const char* filename, unsigned fnsize, int encode_id)
{
	_frameCount = 0;
	if (!FountainInit::init())
	{
		std::cerr << "failed FountainInit :(" << std::endl;
		return -5;
	}

	Encoder enc;
	if (encode_id < 0)
		enc.set_encode_id(++_encodeId); // increment _encodeId every time we change files
	else
		enc.set_encode_id(static_cast<uint8_t>(encode_id));

	cimbar::byte_istream bis(reinterpret_cast<const char*>(buffer), size);
	_fes = enc.create_fountain_encoder(bis, std::string_view(filename, fnsize), _compressionLevel);

	if (!_fes)
		return -1; // return -1 plz

	_next.reset();
	return 0;
}

int cimbare_configure(int mode_val, int compression)
{
	// defaults
	if (mode_val == 0)
		mode_val = 68;
	if (compression < 0 or compression > 22)
		compression = cimbar::Config::compression_level();

	// make sure we've initialized
	int window_size_x = cimbar::Config::image_size_x() + 16;
	int window_size_y = cimbar::Config::image_size_y() + 16;
	int initRes = cimbare_init_window(window_size_x, window_size_y);
	if (initRes < 0)
		return initRes;

	// check if we need to refresh the stream
	bool refresh = (mode_val != _modeVal or compression != _compressionLevel);
	if (refresh)
	{
		// update config
		_modeVal = mode_val;
		_compressionLevel = compression;

		cimbar::Config::update(mode_val);

		// try to refresh the stream
		if (_window and _fes)
		{
			unsigned buff_size_new = cimbar::Config::fountain_chunk_size();
			if (!_fes->restart_and_resize_buffer(buff_size_new))
			{
				// if the data is too small, we should throw out _fes -- and clear the canvas.
				_fes = nullptr;
				_window->clear();
				_next.reset();
			}
			_frameCount = 0;
			_window->shake(0);
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
