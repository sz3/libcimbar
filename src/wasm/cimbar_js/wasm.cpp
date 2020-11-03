/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "encoder/SimpleEncoder.h"
#include "gui/window_glfw.h"
#include "util/byte_istream.h"

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>


namespace {
	std::shared_ptr<cimbar::window_glfw> _window;
	std::shared_ptr<fountain_encoder_stream> _fes;
	int _renders = 0;
	uint8_t _encodeId = 0;

	// settings
	unsigned _ecc = 30;
	unsigned _colorBits = 2;
}

extern "C" {

int initialize_GL(int width, int height)
{
	if (_window)
		return 1;

	std::cerr << "initializing " << width << " by " << height << " window";

	_window = std::make_shared<cimbar::window_glfw>(width, height, "OpenGL test");
	if (!_window or !_window->is_good())
		return 0;

	return 1;
}

int render()
{
	if (!_window or !_fes)
		return 0;

	// we generate 2x the amount of required blocks -- unless everything fits in a single frame.
	unsigned required = _fes->blocks_required();
	if (required > cimbar::Config::fountain_chunks_per_frame())
		required = required*2;
	if (_fes->block_count() > required)
	{
		_fes->reset();
		_window->rotate(0);
	}

	SimpleEncoder enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
	enc.set_encode_id(_encodeId);

	std::optional<cv::Mat> img = enc.encode_next(*_fes);
	if (!img)
	{
		std::cerr << "no image :(" << std::endl;
		return 0;
	}

	_window->show(*img, 0);
	_window->rotate();
	return ++_renders;
}

int encode(uint8_t* buffer, size_t size)
{
	std::cerr << "encode buff size " << size << std::endl;

	if (!FountainInit::init())
		std::cerr << "failed FountainInit :(" << std::endl;

	SimpleEncoder enc(_ecc, cimbar::Config::symbol_bits(), _colorBits);
	enc.set_encode_id(++_encodeId); // increment _encodeId every time we change files

	cimbar::byte_istream bis(reinterpret_cast<char*>(buffer), size);
	_fes = enc.create_fountain_encoder(bis);

	if (!_fes)
		return 0;
	return 1;
}

int configure(unsigned color_bits)
{
	if (color_bits != _colorBits and color_bits <= 3)
	{
		_colorBits = color_bits;
		// TODO: what if the data is too small? do we just destroy the _fes and start over? Don't we kinda have to?
		if (_fes)
			_fes->reset_and_resize_buffer(cimbar::Config::fountain_chunk_size(_ecc, cimbar::Config::symbol_bits() + _colorBits));
	}
	return 0;
}

}
