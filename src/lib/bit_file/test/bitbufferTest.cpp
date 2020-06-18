#include "unittest.h"

#include "bitbuffer.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "bitbufferTest/testSimple.1", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(1, 1, 1);
	bb.write(1, 7, 1);

	assertEquals( 65, (int)bb.buffer()[0] );
	assertEquals( 'A', bb.buffer()[0] );
}

TEST_CASE( "bitbufferTest/testSimple.4", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(0x4, 0, 4);
	bb.write(0x9, 4, 4);

	assertEquals( 73, (int)bb.buffer()[0] );
	assertEquals( 'I', bb.buffer()[0] );
}

TEST_CASE( "bitbufferTest/testSimple.6", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(0x1F, 0, 6);
	bb.write(0x0A, 6, 6);
	bb.write(0x03, 12, 6);
	bb.write(0x11, 18, 6);

	assertEquals( 0x7C, (int)(unsigned char)bb.buffer()[0] );
	assertEquals( 0xA0, (int)(unsigned char)bb.buffer()[1] );
	assertEquals( 0xD1, (int)(unsigned char)bb.buffer()[2] );
}

TEST_CASE( "bitbufferTest/testSimple.7", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(0x68, 0, 7);
	bb.write(0x1F, 7, 7);
	bb.write(0x43, 14, 7);
	bb.write(0x70, 21, 7);

	assertEquals( 0xd0, (int)(unsigned char)bb.buffer()[0] );
	assertEquals( 0x7e, (int)(unsigned char)bb.buffer()[1] );
	assertEquals( 0x1f, (int)(unsigned char)bb.buffer()[2] );
}

TEST_CASE( "bitbufferTest/testSimple.8", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(67, 8, 8);
	assertEquals( 67, (int)bb.buffer()[1] );
	assertEquals( 'C', bb.buffer()[1] );
}

TEST_CASE( "bitbufferTest/testSimple.10", "[unit]" )
{
	bitbuffer<> bb;
	bb.write(355, 2, 10);
	assertEquals( 22, (int)bb.buffer()[0] );
	assertEquals( 48, (int)bb.buffer()[1] );
}
