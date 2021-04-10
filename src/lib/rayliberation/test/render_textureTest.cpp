/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "render_texture.h"
#include "graphics/RayCommon.h"

#include "raylib.h"
#include "serialize/format.h"
#include <iostream>
#include <string>
using std::string;

TEST_CASE( "render_textureTest/testDefault", "[unit]" )
{
	InitWindow(500, 500, "test raylib");
	BeginDrawing();
	ClearBackground(BLACK);

	//cimbar::render_texture rt(1024, 1024, BLACK);

	cimbar::texture tx = cimbar::load_texture("bitmap/4/00.png");

	/*rt.paste(tx, 10, 10, WHITE);
	rt.paste(tx, 20, 20, WHITE);
	rt.draw();*/

	std::cout << fmt::format("what? {},{},{}", tx.tx().id, tx.tx().width, tx.tx().height) << std::endl;

	DrawTexture(tx.tx(), 10, 10, WHITE);
	EndDrawing();

	assertEquals(true, true);

	//*
	for (int i = 0; i < 100000; ++i)
		std::cout << "hello" << i << std::endl;
	//*/
}
