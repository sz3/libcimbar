/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <streambuf>

namespace cimbar {

struct bytebuf : public std::streambuf
{
	bytebuf(char* data, size_t len)
	{
		setg(data, data, data + len);
	}
};

}
