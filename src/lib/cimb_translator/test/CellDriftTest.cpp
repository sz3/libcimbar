/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "CellDrift.h"

#include <iostream>
#include <string>
#include <vector>

TEST_CASE( "CellDriftTest/testSimple", "[unit]" )
{
	CellDrift drift;
	assertEquals( 0, drift.x() );
	assertEquals( 0, drift.y() );

	drift.updateDrift(0, 0);
	assertEquals( 0, drift.x() );
	assertEquals( 0, drift.y() );

	drift.updateDrift(1, 0);
	assertEquals( 1, drift.x() );
	assertEquals( 0, drift.y() );

	for (int i = 0; i < 10; ++i)
		drift.updateDrift(1, 1);
	assertEquals( 2, drift.x() );
	assertEquals( 2, drift.y() );

	drift.updateDrift(-1, 1);
	assertEquals( 1, drift.x() );
	assertEquals( 2, drift.y() );

	for (int i = 0; i < 10; ++i)
		drift.updateDrift(-1, -1);
	assertEquals( -2, drift.x() );
}
