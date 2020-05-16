#include "unittest.h"

#include "fountain_encoder_stream.h"
#include "fountain_decoder_stream.h"

#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "FountainStreamTest/testEncoder", "[unit]" )
{
	FountainInit::init();

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

TEST_CASE( "FountainStreamTest/testDecode", "[unit]" )
{
	FountainInit::init();

	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream fes = fountain_encoder_stream::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	fountain_decoder_stream<> fds(input.str().size());

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes.readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		auto output = fds.write(buff.data(), buff.size());
		if (output)
		{
			assertEquals( input.str().size(), output->size() );
			assertEquals( input.str(), string(output->begin(), output->end()) );
			break;
		}
		else if (i == 999)
			assertMsg((bool)output, "couldn't decode :(");
	}

	assertEquals( 171, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );
}


TEST_CASE( "FountainStreamTest/testDecode_BigPackets", "[unit]" )
{
	FountainInit::init();

	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream fes = fountain_encoder_stream::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	fountain_decoder_stream<> fds(input.str().size());

	std::array<char, 830> buff; // one block per read/write
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes.readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		auto output = fds.write(buff.data(), buff.size());
		if (output)
		{
			assertEquals( input.str().size(), output->size() );
			assertEquals( input.str(), string(output->begin(), output->end()) );
			break;
		}
		else if (i == 999)
			assertMsg((bool)output, "couldn't decode :(");
	}

	assertEquals( 14, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );
}


