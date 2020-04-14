#include "unittest.h"

#include "bitwriter.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "bitwriterTest/testSimple", "[unit]" )
{
	std::string input = "Hello";
	bitwriter br;
	assertEquals( true, br.writeBit(false) );
	assertEquals( true, br.writeBit(true) );
	assertEquals( true, br.writeBit(false) );
	assertEquals( true, br.writeBit(true) );
	assertEquals( true, br.writeBit(true) );
	assertEquals( true, br.writeBit(true) );
	assertEquals( true, br.writeBit(true) );
	assertEquals( true, br.writeBit(true) );

	assertEquals( 95, br.buffer()[0] );
	assertEquals( '_', br.buffer()[0] );
}

TEST_CASE( "bitwriterTest/testWrite", "[unit]" )
{
	bitwriter br;
	br.write(65, 8);
	assertEquals( 65, (int)br.buffer()[0] );
	assertEquals( 'A', br.buffer()[0] );

	br.write(7, 5);
	br.write(2, 3);
	assertEquals( 58, (int)br.buffer()[1] );
}
