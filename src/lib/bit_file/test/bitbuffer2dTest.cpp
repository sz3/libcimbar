/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "bitbuffer2d.h"

TEST_CASE( "bitbuffer2dTest/testSimple.1", "[unit]" )
{
	bitbuffer2d bb(64, 64);
	bitbuffer::writer bw(bb);
	for (int i = 0; i < 64; ++i)
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

	uint64_t res = bb.read(8, 8);
	assertEquals( 66, res );

	res = bb.read(16, 8);
	assertEquals( 67, res );

	// in betweens
	// [8, 20] =>
	// 0100 0010 0100
	res = bb.read(9, 8);
	assertEquals( 132, res );
	res = bb.read(9, 7);
	assertEquals( 66, res );
	res = bb.read(9, 6);
	assertEquals( 33, res );
	res = bb.read(9, 5);
	assertEquals( 16, res );
	res = bb.read(9, 4);
	assertEquals( 8, res );

}

TEST_CASE( "bitbuffer2dTest/testSectorMask.SpotCheck", "[unit]" )
{
	bitbuffer2d bb(64, 64);
	bitbuffer::writer bw(bb);
	for (int i = 0; i < 64; ++i)
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

	// ok, now let's get cute
	// [8, 20] =>
	// 0100 0001 0100
	// (same every row)
	{
		// 10 0000 1010 repeated 10x
		intx::uint128 res = bb.read_sector_mask(1, 1, 10, 10);
		assertEquals( 0xA82A0A82A0A82A0A, res[0] );
		// prev ends with 101010, so for our last 36 we start with 00000
		assertEquals( 0x82A0A82A0, res[1] );
	}
}

TEST_CASE( "bitbuffer2dTest/testSectorMask.Exhaustive1", "[unit]" )
{
	bitbuffer2d bb(64, 64);
	bitbuffer::writer bw(bb);
	for (int i = 0; i < 64; ++i)
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

	// ok, now let's get cute
	// [8, 20] =>
	// 0100 0001 0100
	// (same every row)
	for (int row = 0; row < (64-8); ++row)
	{
		for (int col = 0; col < (64-8); ++col)
		{
			uint64_t mask = (uint64_t)bb.read_sector_mask(col, row, 8, 1);
			uint64_t res = bb.read(col+(row*64), 8);
			assertEquals( res, mask );
		}
	}
}

TEST_CASE( "bitbuffer2dTest/testSectorMask.Exhaustive2", "[unit]" )
{
	bitbuffer2d bb(64, 64);
	bitbuffer::writer bw(bb);
	for (int i = 0; i < 64; ++i)
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

	// ok, now let's get cute
	// [8, 20] =>
	// 0100 0001 0100
	// (same every row)
	for (int col = 0; col < (64-8); ++col)
	{
		uint64_t mask = (uint64_t)bb.read_sector_mask(col, 1, 8, 8);
		uint64_t res = bb.read(col+64, 8);
		for (int i = 0; i < 8; ++i)
			res |= (res << 8);
		assertEquals( res, mask );
	}
}
