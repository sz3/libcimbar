/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "RenderTextureMat.h"

#include "raylib.h"

#include <iostream>
#include <string>
using std::string;

TEST_CASE( "RenderTextureMatTest/testDefault", "[unit]" )
{
	InitWindow(500, 500, "test raylib");

	cimbar::RenderTextureMat rtm(1024, 1024, BLACK);
	assertEquals(true, false);
}
