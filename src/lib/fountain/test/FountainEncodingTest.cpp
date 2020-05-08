#include "unittest.h"

#include "fountain/FountainDecoder.h"
#include "fountain/FountainEncoder.h"
#include "serialize/format.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE( "FountainEncodingTest/testEncoder", "[unit]" )
{
	FountainEncoder<1120>::init();

	std::string content;
	for (int i = 0; i < 1000; ++i)
		content += "0123456789";
	FountainEncoder<1120> encoder(content.data(), content.size());
	assertTrue( encoder.good() );

	FountainEncoder<1120>::buffer buff;
	for (int i = 0; i < 500; ++i)
		DYNAMIC_SECTION( i << ":" )
		{
			if (i == 8)  // last of "unencoded" blocks
				assertEquals( 1040, encoder.encode(i, buff) );
			else
				assertEquals( 1120, encoder.encode(i, buff) );
		}
}

TEST_CASE( "FountainEncodingTest/testEncodingAndDecoding", "[unit]" )
{
	FountainDecoder<1120>::init();
	FountainEncoder<1120>::init();

	std::string content;
	for (int i = 0; i < 1000; ++i)
		content += "0123456789";

	FountainEncoder<1120> encoder(content.data(), content.size());
	assertTrue( encoder.good() );

	FountainDecoder<1120> decoder(content.size());
	assertTrue( decoder.good() );

	FountainEncoder<1120>::buffer buff;
	for (int i = 0; i < 20; ++i)
		DYNAMIC_SECTION( i << ":" )
		{
			if (i == 8)  // last of "unencoded" blocks
			{
				assertEquals( 1040, encoder.encode(i, buff) );
				assertEquals( std::nullopt, decoder.decode(i, buff.data(), 1040) );
			}
			else
			{
				assertEquals( 1120, encoder.encode(i, buff) );
				assertEquals( std::nullopt, decoder.decode(i, buff.data(), 1120) );
			}
		}
}
