/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "FountainDecoder.h"
#include "FountainEncoder.h"
#include "serialize/format.h"

#include "base91/base.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

namespace {
	std::vector<std::string> blocks()
	{
		std::vector<std::string> precomp = {
		    R"(gw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77B)",
		    R"(b%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@hN7R;4!x;m?zs12+Xv$Sb%ob^heMAJ-:N[*T6texAkZdx2G?uCbRA2IbLmUuFKR*|jgSr!=EWo!G-1m/=*lT{+(abj5XgJ8=%N|QQztEml0o[2;(8YAS77Z.wnXB7IB.y6FTL)CF2i~RR2W-mCLUgw_,$k_i$Js@A)",
		    R"(c%Uhlr*CCfs)S[@q4^ELGi@D*2w[l$TsWuwP'mG44=-{`EM!`%Vsmoc=+Ts)%^_{8,QnK??D;$8=3Q%&juuPX?2op&:{vFE!{uj.+`T=JWM)*HtTE//mn}qCGf~=KWU%8^@JWi\pr}-@2$yn{um=-^c1h\S))H5nc%Uhlr*CCfs)S[@q4^ELGi@D*2w[l$TsWuwP'mG44=-{`EM!`%Vsmoc=+Ts)%^_{8,QnK??D;$8=3Q%&juuPX?2op&:{vFE!{uj.+`T=JWM)*HtTE//mn}qCGf~=KWU%8^@JWi\pr}-@2$yn{um=-^c1h\S))H5nc%Uhlr*CCfs)S[@q4^ELGi@D*2w[l$TsWuwP'mG44=-{`EM!`%Vsmoc=+Ts)%^_{8,QnK??D;$8=3Q%&juuPX?2op&:{vFE!{uj.+`T=JWM)*HtTE//mn}qCGf~=KWU%8^@JWi\pr}-@9j[%o[##DKr^90w^%3*:Oo;#9eG$=q'c5+Y;!gV-j[{ZpMCf(o?iu3.N:xGi`.qx'pR3TTYu!V-R|=%o\-BF){f,*p?.ilHU_-ePE=mVu7d@5lg9Km$`~5Vzv8i%o!*.]z]YE!r[(*'U2b3JN)=3^I'^gUnmuCir9j[%o[##DKr^90w^%3*:Oo;#9eG$=q'c5+Y;!gV-j[{ZpMCf(o?iu3.N:xGi`.qx'pR3TTYu!V-R|=%o\-BF){f,*p?.ilHU_-ePE=mVu7d@5lg9Km$`~5Vzv8i%o!*.]z]YE!r[(*'U2b3JN)=3^ID)"
		};
		return precomp;
	}
}

TEST_CASE( "FountainEncodingTest/testEncoder", "[unit]" )
{
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

	unsigned messageSize = 10000;
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
	unsigned decoded_blocks = 0;
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
	assertEquals( decoded_blocks, decoder.progress() );
}

TEST_CASE( "FountainEncodingTest/testConsistency", "[unit]" )
{
	const std::vector<std::string> precomputed = blocks();
	static const unsigned packetSize = 626;

	unsigned messageSize = 1000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	// make sure encoding is consistent
	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	std::array<uint8_t,packetSize> block;

	for (unsigned block_id = 0; block_id < 3; ++block_id)
	{
		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		std::string actual = base91::encode(std::string((char*)block.data(), bites));
		//std::cout << block_id << " : " << actual << std::endl;
		assertEquals( precomputed[block_id], actual );
	}

	// make sure decoding is consistent
	FountainDecoder decoder(messageSize, packetSize);
	for (unsigned block_id = 0; block_id < 3; ++block_id)
	{
		if (block_id == 1)
			continue;
		std::string block_data = base91::decode(precomputed[block_id]);
		std::optional<vector<uint8_t>> res = decoder.decode(block_id, (uint8_t*)block_data.data(), block_data.size());
		if (block_id == 2)
		{
			assertTrue( res );
			std::string actual = std::string((char*)res->data(), res->size());
			assertEquals( message, actual );
		}
	}
}

TEST_CASE( "FountainEncodingTest/testWhichN", "[unit]" )
{
	const std::vector<std::string> precomputed = blocks();
	static const unsigned packetSize = 624;

	unsigned messageSize = 6000;
	std::string message;
	while (message.size() < messageSize)
		message += "0123456789";
	message.resize(messageSize);

	// create encoder and decoder
	FountainEncoder encoder((uint8_t*)message.data(), message.size(), packetSize);
	FountainDecoder decoder(messageSize, packetSize);

	std::array<uint8_t,packetSize> block;
	std::vector<uint8_t> final_result;

	// decode backwards, never using the "real" data blocks
	int block_id = 105;
	for (; block_id >= 0; block_id -= 1)
	{
		unsigned bites = encoder.encode(block_id, block.data(), block.size());
		std::optional<vector<uint8_t>> res = decoder.decode(block_id, block.data(), bites);
		if (res)
		{
			final_result = *res;
			break;
		}
	}

	assertEquals( 96, block_id );
	assertEquals( message, string((char*)final_result.data(), final_result.size()) );
	assertEquals( 10, decoder.progress() );
}

