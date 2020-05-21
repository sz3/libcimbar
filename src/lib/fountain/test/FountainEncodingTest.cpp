#include "unittest.h"

#include "FountainDecoder.h"
#include "FountainEncoder.h"
#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

TEST_CASE( "FountainEncodingTest/testEncoder", "[unit]" )
{
	FountainEncoder::init();

	std::string content;
	for (int i = 0; i < 1000; ++i)
		content += "0123456789";

	FountainEncoder encoder((uint8_t*)content.data(), content.size(), 1120);
	assertTrue( encoder.good() );

	std::array<uint8_t,1120> buff;
	for (int i = 0; i < 500; ++i)
		DYNAMIC_SECTION( i << ":" )
		{
			if (i == 8)  // last of "unencoded" blocks
				assertEquals( 1040, encoder.encode(i, buff.data(), buff.size()) );
			else
				assertEquals( 1120, encoder.encode(i, buff.data(), buff.size()) );
		}
}

TEST_CASE( "FountainEncodingTest/testEncodingAndDecoding", "[unit]" )
{
	static const unsigned packetSize = 1400;
	FountainDecoder::init();
	FountainEncoder::init();

	int messageSize = 10000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	assertTrue( encoder.good() );

	FountainDecoder decoder(message.size(), packetSize);
	assertTrue( decoder.good() );

	std::array<uint8_t,packetSize> block;
	int block_id = 0;
	int decoded_blocks = 0;
	for (; block_id < 50; ++block_id)
	{
		if (block_id % 3 == 0)  // 33% packet loss
			continue;
		++decoded_blocks;

		unsigned expectedSize = (block_id == 7)? 200 : 1400;
		assertEquals( expectedSize, encoder.encode(block_id, block.data(), block.size()) );
		if (block_id != 11)
		{
			assertMsg( std::nullopt == decoder.decode(block_id, block.data(), expectedSize), fmt::format("block {}", block_id) );
			assertEquals( 1, decoder.last_result() );
		}
		else
		{
			std::optional<vector<uint8_t>> res = decoder.decode(block_id, block.data(), expectedSize);
			assertTrue( res );

			vector<uint8_t>& out = *res;
			vector<uint8_t> expected = std::vector<uint8_t>(message.begin(), message.end());
			assertEquals( messageSize, out.size() );
			assertEquals( expected, out );
			break;
		}
	}

	assertEquals( 11, block_id );
	assertEquals( 8, decoded_blocks );
}
