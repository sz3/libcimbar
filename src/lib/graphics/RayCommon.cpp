/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "RayCommon.h"

#include "base91/base.hpp"
#include "serialize/format.h"
#include "rayliberation/texture.h"

#include <map>
#include <string>
#include "bitmaps.h"

using std::string;

namespace {
	string load_file(string path)
	{
		auto it = cimbar::bitmaps.find(path);
		if (it == cimbar::bitmaps.end())
			return "";

		return base91::decode(it->second);
	}
}

namespace cimbar {

cimbar::texture load_texture(string path)
{
	string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	return cimbar::texture(LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size()), WHITE);
}

}
