/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "render_texture.h"
#include "texture.h"
#include "RayCommon.h"

#include "raylib.h"
#include "serialize/format.h"
#include <iostream>
#include <string>
using std::string;

TEST_CASE( "render_textureTest/testDefault", "[unit]" )
{
	InitWindow(1024, 1024, "test raylib");
	cimbar::render_texture rt(1024, 1024, BLACK);
	cimbar::texture tx = cimbar::load<cimbar::texture>::load_img("bitmap/4/00.png");

	rt.paste(tx, 10, 10);

	tx.copyTo(rt({20, 20, tx.cols, tx.rows}));

	BeginDrawing();
	ClearBackground(BLACK);
	rt.draw();

	DrawTexture(tx.tx(), 100, 90, WHITE);

	EndDrawing();

	assertEquals(true, true);

	//*
	for (int i = 0; i < 100000; ++i)
		std::cout << "hello" << i << std::endl;
	//*/
}
