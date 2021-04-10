/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "render_texture.h"

#include "raylib.h"

#include <iostream>
#include <string>
using std::string;

TEST_CASE( "render_textureTest/testDefault", "[unit]" )
{
	InitWindow(500, 500, "test raylib");

	cimbar::render_texture rt(1024, 1024, BLACK);
	assertEquals(true, false);
}
