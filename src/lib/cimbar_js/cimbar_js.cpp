/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "cimbar_js.h"

#include "encoder/SimpleEncoder.h"
#include "gui/window.h"
#include "util/byte_istream.h"

#include <iostream>
#include <sstream>

using EncoderType = SimpleEncoder<cimbar::image, cimbar::frame>;

namespace {
	std::shared_ptr<cimbar::window> _window;
	std::shared_ptr<fountain_encoder_stream> _fes;
	std::optional<cimbar::frame> _next;

	int _frameCount = 0;
	uint8_t _encodeId = 0;

	// settings
	unsigned _ecc = 30;
	unsigned _colorBits = 2;
	int _compressionLevel = 6;
}

extern "C" {

int initialize_GL(int width, int height)
{
	if (_window)
		return 1;

	std::cerr << "initializing " << width << " by " << height << " window";

	_window = std::make_shared<cimbar::window>(width, height, "Cimbar Encoder");
	if (!_window or !_window->is_good())
		return 0;

	return 1;
}

// render() and next_frame() could be put in the same function,
// but it seems cleaner to split them.
// in any case, we're concerned with frame pacing (some encodes take longer than others)
int render()
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

int next_frame()
{
	if (!_window or !_fes)
		return 0;

	// we generate 8x the amount of required blocks -- unless everything fits in a single frame.
	unsigned required = _fes->blocks_required();
	if (required > cimbar::Config::fountain_chunks_per_frame())
		required = required*8;
	if (_fes->block_count() > required)
	{
		_fes->restart();
		_window->shake(0);
		_frameCount = 0;
	}

	EncoderType enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
	enc.set_encode_id(_encodeId);

	_next = enc.encode_next(*_fes, _window->width());
	return ++_frameCount;
}

int encode(unsigned char* buffer, unsigned size, int encode_id)
{
	_frameCount = 0;
	if (!FountainInit::init())
		std::cerr << "failed FountainInit :(" << std::endl;

	EncoderType enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
	if (encode_id < 0)
		enc.set_encode_id(++_encodeId); // increment _encodeId every time we change files
	else
		enc.set_encode_id(static_cast<uint8_t>(encode_id));

	cimbar::byte_istream bis(reinterpret_cast<char*>(buffer), size);
	_fes = enc.create_fountain_encoder(bis, _compressionLevel);

	if (!_fes)
		return 0;

	_next.reset();
	return 1;
}

int configure(unsigned color_bits, unsigned ecc, int compression)
{
	// defaults
	if (color_bits > 3)
		color_bits = 2;
	if (ecc < 0 or ecc >= 150)
		ecc = 30;
	if (compression < 0 or compression > 22)
		compression = 6;

	// check if we need to refresh the stream
	bool refresh = false;
	if (color_bits != _colorBits)
	{
		_colorBits = color_bits;
		refresh = true;
	}
	if (ecc != _ecc)
	{
		_ecc = ecc;
		refresh = true;
	}
	if (compression != _compressionLevel)
		_compressionLevel = compression;

	// try to refresh the stream
	if (refresh)
	{
		if (_window and _fes)
		{
			unsigned buff_size_new = cimbar::Config::fountain_chunk_size(_ecc, cimbar::Config::symbol_bits() + _colorBits);
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

}
