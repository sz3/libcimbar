/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "nlargest.h"


TEST_CASE( "nlargestTest/testMinMax", "[unit]" )
{
	nlargest<uint8_t, 3> ns;

	// defaults
	assertEquals(0, (int)ns.min());
	assertEquals(0, (int)ns.max());

	ns.eval(150);
	ns.eval(100);
	ns.eval(200);

	assertEquals(100, (int)ns.min());
	assertEquals(200, (int)ns.max());

	ns.eval(200);
	ns.eval(170);

	assertEquals(170, (int)ns.min());
	assertEquals(200, (int)ns.max());
}

