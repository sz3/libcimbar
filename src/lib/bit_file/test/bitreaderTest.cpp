#include "unittest.h"

#include "bitreader.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "bitreaderTest/testSimple", "[unit]" )
{
	std::string input = "Hello world";
	bitreader br(input.data(), input.size());
	assertEquals( 0x48, br.read(8) );
	assertEquals( 0x65, br.read(8) );
	assertEquals( 0x6c, br.read(8) );
	assertEquals( 0x6c, br.read(8) );
	assertEquals( 0x6f, br.read(8) );
	assertEquals( 0x20, br.read(8) );
}

TEST_CASE( "bitreaderTest/testDefault", "[unit]" )
{
	std::string input = "Hello world";
	bitreader br(input.data(), input.size());

	std::vector<unsigned> res;
	while (!br.empty())
		res.push_back(br.read(8));

	std::vector<unsigned> expected({0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64});
	assertEquals(expected, res);
}

TEST_CASE( "bitreaderTest/testPartial", "[unit]" )
{
	std::string input = "Hello world";
	bitreader br(input.data(), input.size());

	std::vector<unsigned> res;
	while (!br.empty())
		res.push_back(br.read(5));

	std::vector<unsigned> expected({9, 1, 18, 22, 24, 27, 3, 15, 4, 1, 27, 22, 30, 28, 19, 12, 12, 16});
	assertEquals(expected, res);
}
