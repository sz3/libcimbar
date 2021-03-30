/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
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
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	assertEquals( 170, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_BlockHeader", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 636);

	assertEquals( 0, fes->block_count() );
	assertEquals( 16, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 636> buff;
	for (unsigned i = 0; i < 20; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );

		// encode_id
		assertEquals( 0, buff[0] );

		// total size == 10000
		assertEquals( 0, buff[1] );
		assertEquals( 39, (unsigned)buff[2] );
		assertEquals( 16, buff[3] );

		// block_id
		assertEquals( 0, buff[4] );
		if (i+1 >= fes->blocks_required())
			assertEquals( i+1, (unsigned)buff[5] );
		else
			assertEquals( i, (unsigned)buff[5] );
	}

	assertEquals( 21, fes->block_count() );
	assertEquals( 16, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_DifferentBuffSizes", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	stringstream input2;
	input2 << input.str();

	fountain_encoder_stream::ptr fes1 = fountain_encoder_stream::create(input, 830);
	fountain_encoder_stream::ptr fes2 = fountain_encoder_stream::create(input2, 830);

	stringstream oneforty;
	std::array<char, 140> buff1;
	for (int i = 0; i < 83; ++i)
	{
		unsigned res = fes1->readsome(buff1.data(), buff1.size());
		assertEquals( res, buff1.size() );
		oneforty << string(buff1.begin(), buff1.end());
	}

	stringstream full;
	std::array<char, 830> buff2;
	for (int i = 0; i < 14; ++i)
	{
		unsigned res = fes2->readsome(buff2.data(), buff2.size());
		assertEquals( res, buff2.size() );
		full << string(buff2.begin(), buff2.end());
	}

	assertEquals( full.str(), oneforty.str() );

	assertEquals( 15, fes1->block_count() );
	assertEquals( 15, fes2->block_count() );
}


TEST_CASE( "FountainStreamTest/testEncoder_Consistency", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 100; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 400);

	stringstream full;
	std::array<char, 400> buff;

	unsigned res = fes->readsome(buff.data(), buff.size());
	assertEquals( res, buff.size() );
	full << string(buff.begin(), buff.end());

	std::string expected = {0, 0, 0x03, (char)0xe8, 0, 0};
	expected += input.str().substr(0, 394);

	assertEquals( expected, full.str() );
}

TEST_CASE( "FountainStreamTest/testEncoder_ChangeBufferSize", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);
	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	assertTrue( fes->restart_and_resize_buffer(600) );

	// changes block params (and internal buffer size)
	assertEquals( 0, fes->block_count() );
	assertEquals( 17, fes->blocks_required() );
	assertTrue( fes->good() );

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
		assertEquals( res, buff.size() );
	}

	assertEquals( 235, fes->block_count() );
	assertEquals( 17, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testEncoder_ChangeBufferSize_Fails", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 100; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 800);
	assertEquals( 0, fes->block_count() );
	assertEquals( 2, fes->blocks_required() );
	assertTrue( fes->good() );

	assertFalse( fes->restart_and_resize_buffer(1200) ); // larger than the buffer

	// stream left unchanged
	assertEquals( 0, fes->block_count() );
	assertEquals( 2, fes->blocks_required() );
	assertTrue( fes->good() );
}


TEST_CASE( "FountainStreamTest/testDecode", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	fountain_decoder_stream fds(input.str().size(), 830);

	std::array<char, 140> buff;
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
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

	assertEquals( 824, fds.block_size() );
	assertEquals( 10000, fds.data_size() );
	assertTrue( fds.good() );
	assertEquals( fes->blocks_required(), fds.blocks_required() );
	assertEquals( fes->blocks_required(), fds.progress() );

	assertEquals( 15, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );
}

TEST_CASE( "FountainStreamTest/testDecode_BigPackets", "[unit]" )
{
	stringstream input;
	for (int i = 0; i < 1000; ++i)
		input << "0123456789";

	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 830);

	assertEquals( 0, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

	fountain_decoder_stream fds(input.str().size(), 830);

	std::array<char, 830> buff; // one block per read/write
	for (int i = 0; i < 1000; ++i)
	{
		unsigned res = fes->readsome(buff.data(), buff.size());
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

	assertEquals( 824, fds.block_size() );
	assertEquals( 10000, fds.data_size() );
	assertTrue( fds.good() );

	assertEquals( 14, fes->block_count() );
	assertEquals( 13, fes->blocks_required() );
	assertTrue( fes->good() );

}

