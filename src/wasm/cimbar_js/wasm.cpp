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
	uint8_t _encodeId = 0;
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

void render()
{
	if (!_window or !_fes)
		return;

	// this should probably be x2 (at least), but for now...
	if (_fes->block_count() > _fes->blocks_required()*2)
		_fes->reset();

	SimpleEncoder enc(30);
	enc.set_encode_id(_encodeId);

	std::optional<cv::Mat> img = enc.encode_next(*_fes);
	if (!img)
	{
		std::cerr << "no image :(" << std::endl;
		return;
	}

	_window->show(*img, 0);
}

int encode(uint8_t* buffer, size_t size)
{
	std::cerr << "encode buff size " << size << std::endl;

	if (!FountainInit::init())
		std::cerr << "failed FountainInit :(" << std::endl;

	SimpleEncoder enc(30);
	enc.set_encode_id(++_encodeId); // increment _encodeId every time we change files

	cimbar::byte_istream bis(reinterpret_cast<char*>(buffer), size);
	_fes = enc.create_fountain_encoder(bis);

	if (!_fes)
		return 0;
	return 1;
}

}
