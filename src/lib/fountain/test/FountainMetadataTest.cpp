/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "FountainMetadata.h"

namespace {
	inline constexpr unsigned char operator"" _uchar(unsigned long long arg) noexcept
	{
		return static_cast<unsigned char>(arg);
	}
}

TEST_CASE( "FountainMetadataTest/testFromBytes", "[unit]" )
{
	std::string buff = {5, 15, 15, 15, 0, 0, 0, 0};
	FountainMetadata inmd(buff.data(), buff.size());
	assertEquals( 5, inmd.encode_id() );
	assertEquals( 0x0f0f0f, inmd.file_size() );
}

TEST_CASE( "FountainMetadataTest/testFromBytes.Short", "[unit]" )
{
	std::string buff = {10, 7, 8, 9};
	FountainMetadata inmd(buff.data(), buff.size());
	assertEquals( 10, inmd.encode_id() );
	assertEquals( 0x070809, inmd.file_size() );
}

TEST_CASE( "FountainMetadataTest/testFromBytes.BigFile", "[unit]" )
{
	std::string buff = {(char)0x81, 7, 8, 9};
	FountainMetadata inmd(buff.data(), buff.size());
	assertEquals( 1, inmd.encode_id() );
	assertEquals( 0x1070809, inmd.file_size() );
}

TEST_CASE( "FountainMetadataTest/testThings", "[unit]" )
{
	FountainMetadata inmd(0_uchar, 0xd0731f, 0U);
	assertEquals( 0, inmd.encode_id() );
	assertEquals( 0xd0731f, inmd.file_size() );

	FountainMetadata outmd(inmd.id());
	assertEquals( inmd.encode_id(), outmd.encode_id() );
	assertEquals( inmd.file_size(), outmd.file_size() );
}

TEST_CASE( "FountainMetadataTest/testIgnoreTopEncodeIdBit", "[unit]" )
{
	FountainMetadata inmd(250_uchar, 0xFFFFFF, 0U);
	assertEquals( 122, static_cast<unsigned>(inmd.encode_id()) );
	assertEquals( 0xFFFFFF, inmd.file_size() );

	FountainMetadata outmd(inmd.id());
	assertEquals( inmd.encode_id(), outmd.encode_id() );
	assertEquals( inmd.file_size(), outmd.file_size() );
}

TEST_CASE( "FountainMetadataTest/testBigFile", "[unit]" )
{
	FountainMetadata inmd(2_uchar, 0x1FFFFFF, 0U);
	assertEquals( 2, static_cast<unsigned>(inmd.encode_id()) );
	assertEquals( 0x1FFFFFF, inmd.file_size() );

	FountainMetadata outmd(inmd.id());
	assertEquals( inmd.encode_id(), outmd.encode_id() );
	assertEquals( inmd.file_size(), outmd.file_size() );
}

