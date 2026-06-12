/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "bitbuffer2d.h"

TEST_CASE( "bitbuffer2dTest/testSimple.1", "[unit]" )
{
	bitbuffer2d bb(64, 64);
	bitbuffer::writer bw(bb);
	for (int i = 0; i < 8; ++i)
	{
		bw << 'A';
		bw << 'B';
		bw << 'C';
		bw << 'D';
		bw << 'E';
		bw << 'F';
		bw << 'G';
		bw << 'H';
	}

	assertEquals( 65, (int)bb.buffer()[0] );
	assertEquals( 'A', bb.buffer()[0] );
}
