/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "RayCommon.h"
#include "cimb_translator/res.h"

#include "base91/base.hpp"
#include "serialize/format.h"
#include "rayliberation/texture.h"

using std::string;

namespace cimbar {

cimbar::texture load_texture(string path)
{
	string bytes = load_file(path);
	if (bytes.empty())
		return cimbar::texture();

	return cimbar::texture(LoadImageFromMemory("png", reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size()), WHITE);
}

}
