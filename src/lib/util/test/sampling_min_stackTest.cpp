/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "sampling_min_stack.h"

TEST_CASE( "sample_min_stackTest/testDefault", "[unit]" )
{
	sampling_min_stack<uint16_t> blue;
	assertEquals(255, blue.avg());

	blue += 200;
	blue += 200;
	blue += 200;
	blue += 200;
	assertEquals(200, blue.avg());
	assertEquals(0, blue.total());

	blue += 100;
	blue += 250;
	blue += 100;
	blue += 250;
	assertEquals(900, blue.total());

	blue += 220;
	assertEquals(1120, blue.total());

	blue += 60;
	assertEquals(1320, blue.total());
}
