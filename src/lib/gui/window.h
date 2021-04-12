/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#ifdef LIBCIMBAR_USE_RAYLIB
#include "rayliberation/window_raylib.h"
namespace cimbar {
	using window = window_raylib;
}
#else
#include "gui/window_glfw.h"
namespace cimbar {
	using window = window_glfw;
}
#endif
