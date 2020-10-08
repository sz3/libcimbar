/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "bytebuf.h"
#include <istream>

namespace cimbar {

struct byte_istream : std::istream
{
	byte_istream(const char* data, unsigned len)
	    : _buf((char*)data, len)
	    , std::istream(&_buf)
	{
	}

	bytebuf _buf;
};

}
