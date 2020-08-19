/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "bitreader.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "bitreaderTest/testSimple", "[unit]" )
{
	std::string input = "Hello ";
	bitreader br(input.data(), input.size());
	assertEquals( 0x48, br.read(8) );
	assertEquals( 0x65, br.read(8) );
	assertEquals( 0x6c, br.read(8) );
	assertEquals( 0x6c, br.read(8) );
	assertEquals( 0x6f, br.read(8) );
	assertEquals( 0x20, br.read(8) );

	assertTrue( br.empty() );
	assertEquals( 0, br.read(8) );
}

TEST_CASE( "bitreaderTest/testDefault", "[unit]" )
{
	std::string input = "Hello world";
	bitreader br(input.data(), input.size());

	std::vector<unsigned> res;
	while (!br.empty())
	{
		unsigned bits;
		assertEquals( 8, br.read(8, bits) );
		res.push_back(bits);
	}

	std::vector<unsigned> expected({0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64});
	assertEquals(expected, res);
}

TEST_CASE( "bitreaderTest/testPartialBytes", "[unit]" )
{
	std::string input = "Hello wrld";
	bitreader br(input.data(), input.size());

	std::vector<unsigned> res;
	while (!br.empty())
	{
		unsigned bits;
		assertEquals( 5, br.read(5, bits) );
		res.push_back(bits);
	}

	std::vector<unsigned> expected({9, 1, 18, 22, 24, 27, 3, 15, 4, 1, 27, 23, 4, 27, 3, 4});
	assertEquals(expected, res);
}

TEST_CASE( "bitreaderTest/testCutoff", "[unit]" )
{
	std::string input = "ono";
	bitreader br(input.data(), input.size());

	unsigned bits;
	assertEquals( 5, br.read(5, bits) );
	assertEquals( 13, bits );
	assertEquals( 5, br.read(5, bits) );
	assertEquals( 29, bits );
	assertEquals( 5, br.read(5, bits) );
	assertEquals( 23, bits );
	assertEquals( 5, br.read(5, bits) );
	assertEquals( 6, bits );
	assertEquals( 4, br.read(5, bits) );
	assertEquals( 30, bits );

	assertTrue( br.empty() );
	assertEquals( 0, br.read(5, bits) );
	assertEquals( 0, bits );
}

TEST_CASE( "bitreaderTest/testCutoffAndResume", "[unit]" )
{
	// by using the read(unsigned) function and assign_new_buffer(), we can merge partial reads across buffers
	std::string input = "o";
	bitreader br(input.data(), input.size());

	assertEquals( 13, br.read(5) );
	assertEquals( 28, br.read(5) ); // 2nd read is partial and should be ignored for now
	assertEquals( 3, br.partial() ); // number of bits in the partial read
	assertTrue( br.empty() );

	std::string second = "no";
	br.assign_new_buffer(second.data(), second.size());

	assertEquals( 29, br.read(5) ); // next read adds the partial to the bits read from the new buffer
	assertEquals( 23, br.read(5) );
	assertEquals( 6, br.read(5) );
	assertEquals( 0, br.partial() );
	assertEquals( 30, br.read(5) ); // again
	assertEquals( 4, br.partial() );

	assertTrue( br.empty() );
}
