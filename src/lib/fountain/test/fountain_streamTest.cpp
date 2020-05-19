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

	fountain_encoder_stream fes = fountain_encoder_stream<830>::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes.readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	assertEquals( 170, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_Consistency", "[unit]" )
{
	FountainInit::init();

	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	stringstream input2;
	input2 << input.str();

	fountain_encoder_stream fes1 = fountain_encoder_stream<830>::create(input);
	fountain_encoder_stream fes2 = fountain_encoder_stream<830>::create(input2);

	stringstream oneforty;
	std::array<char, 140> buff1;
	for (int i = 0; i < 83; ++i)
	{
		unsigned res = fes1.readsome(buff1.data(), buff1.size());
		assertEquals( res, buff1.size() );
		oneforty << string(buff1.begin(), buff1.end());
	}

	stringstream full;
	std::array<char, 830> buff2;
	for (int i = 0; i < 14; ++i)
	{
		unsigned res = fes2.readsome(buff2.data(), buff2.size());
		assertEquals( res, buff2.size() );
		full << string(buff2.begin(), buff2.end());
	}

	assertEquals( full.str(), oneforty.str() );

	assertEquals( 15, fes1.block_count() );
	assertEquals( 15, fes2.block_count() );
}

TEST_CASE( "FountainStreamTest/testDecode", "[unit]" )
{
	FountainInit::init();

	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream fes = fountain_encoder_stream<830>::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	fountain_decoder_stream<830> fds(input.str().size());

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

	assertEquals( 15, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );
}


TEST_CASE( "FountainStreamTest/testDecode_BigPackets", "[unit]" )
{
	FountainInit::init();

	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream fes = fountain_encoder_stream<830>::create(input);

	assertEquals( 0, fes.block_count() );
	assertEquals( 13, fes.blocks_required() );
	assertTrue( fes.good() );

	fountain_decoder_stream<830> fds(input.str().size());

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


