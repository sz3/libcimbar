/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "sliding_window.h"


TEST_CASE( "sliding_windowTest/testDefaults", "[unit]" )
{
	sliding_window<uint8_t> window;

	// defaults
	assertEquals(150, (int)window.min());
	assertEquals(150, (int)window.max());
	assertEquals(150, (int)window.mean());
}

TEST_CASE( "sliding_windowTest/testSlide", "[unit]" )
{
	sliding_window<uint8_t> window;

	for (int i = 0; i < 10; ++i)
		window.add(0);

	assertEquals(0, (int)window.min());
	assertEquals(0, (int)window.max());
	assertEquals(0, (int)window.mean());

	for (int i = 0; i < 5; ++i)
		window.add(200);

	assertEquals(0, (int)window.min());
	assertEquals(200, (int)window.max());
	assertEquals(100, (int)window.mean());

	for (int i = 0; i < 4; ++i)
		window.add(200);

	assertEquals(0, (int)window.min());
	assertEquals(200, (int)window.max());
	assertEquals(180, (int)window.mean());

	window.add(200);

	assertEquals(200, (int)window.min());
	assertEquals(200, (int)window.max());
	assertEquals(200, (int)window.mean());
}

