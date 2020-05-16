#include "unittest.h"

#include "fountain_encoder_stream.h"

#include "FountainDecoder.h"
#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "FountainStreamTest/testEncoder", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream fes = fountain_encoder_stream::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes.readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	assertEquals( 171, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );
}

