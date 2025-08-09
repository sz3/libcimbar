/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "zstd/zstd.h"
#include <string>

namespace cimbar {

class zstd_header_check
{
public:
	static std::string get_filename(const unsigned char* data, size_t len)
	{
		if (!ZSTD_isSkippableFrame(data, len))
			return "";

		std::string res;
		res.resize(500, '\0');
		size_t sz = ZSTD_readSkippableFrame(res.data(), res.size(), nullptr, data, len);
		if (sz <= 1)
			return "";

		switch (res[0])
		{
			case 1:
				return std::string(&res[1], sz-1);
			default:
				break;
		}
		return "";
	}

};

}
