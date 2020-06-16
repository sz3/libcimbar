#include "unittest.h"

#include "FountainMetadata.h"
#include "fountain_encoder_stream.h"
#include "fountain_decoder_sink.h"

#include "serialize/format.h"
#include "util/File.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using namespace std;

namespace {
	stringstream dummyContents(unsigned size)
	{
		stringstream input;
		for (int i = 0; i < (size/10); ++i)
			input << "0123456789";
		return input;
	}

	string createFrame(fountain_encoder_stream<626>& fes, string name)
	{
		fes.encode_metadata_block(name);

		stringstream ss;
		std::array<char, 115> buff;

		// for a 6900 byte frame, 115*60 does the trick
		// 626*11 + 14 byte md block
		for (int i = 0; i < 60; ++i)
		{
			unsigned res = fes.readsome(buff.data(), buff.size());
			assertEquals( res, buff.size() );
			ss << string(buff.data(), buff.size());
		}
		return ss.str();
	}

	string createFrame(string name, unsigned size)
	{
		stringstream input = dummyContents(size);
		fountain_encoder_stream fes = fountain_encoder_stream<626>::create(input);
		return createFrame(fes, name);
	}
}

TEST_CASE( "FountainSinkTest/testDefault", "[unit]" )
{
	FountainInit::init();

	fountain_decoder_sink<626> sink("/tmp");
	string iframe = createFrame("theBeginning", 1200);
	assertEquals( 6900, iframe.size() );

	FountainMetadata md(iframe.data(), iframe.size());
	assertEquals( 1200, md.file_size() );
	assertEquals( "theBeginni", md.name() );

	assertEquals( true, sink.decode_frame(iframe.data(), iframe.size()) );
	assertEquals( true, sink.is_done("theBeginni", 1200) );
	assertEquals( false, sink.decode_frame(iframe.data(), iframe.size()) );

	string frame2 = createFrame("another", 1600);
	assertEquals( true, sink.decode_frame(frame2.data(), frame2.size()) );

	string expectedName = "another";
	std::array<uint8_t,10> nameBuff = {0};
	std::copy(expectedName.begin(), expectedName.end(), nameBuff.data());
	expectedName = string(nameBuff.begin(), nameBuff.end());
	assertEquals( true, sink.is_done(expectedName, 1600) );

	assertEquals( 0, sink.num_streams() );
	assertEquals( 2, sink.num_done() );

	string contents = File("/tmp/theBeginni").read_all();
	assertEquals( 1200, contents.size() );
	contents = File("/tmp/another").read_all();
	assertEquals( 1600, contents.size() );
}

TEST_CASE( "FountainSinkTest/testMultipart", "[unit]" )
{
	FountainInit::init();

	fountain_decoder_sink<626> sink("/tmp");

	stringstream input = dummyContents(20000);
	fountain_encoder_stream fes = fountain_encoder_stream<626>::create(input);

	string expectedName = "testMultip";
	for (int i = 0; i < 4; ++i)
	{
		string iframe = createFrame(fes, "testMultipart");
		assertEquals( 6900, iframe.size() );

		FountainMetadata md(iframe.data(), iframe.size());
		assertEquals( 20000, md.file_size() );
		assertEquals( expectedName, md.name() );

		assertMsg( ((i == 2) == sink.decode_frame(iframe.data(), iframe.size())), fmt::format("failed {}", i) );
		assertMsg( ((i >= 2) == sink.is_done(expectedName, 20000)), fmt::format("failed {}", i) );
	}

	assertEquals( 0, sink.num_streams() );
	assertEquals( 1, sink.num_done() );

	string contents = File("/tmp/testMultip").read_all();
	assertEquals( 20000, contents.size() );
}

TEST_CASE( "FountainSinkTest/testSameFrameManyTimes", "[unit]" )
{
	// this is the current test case for what is either a wirehair bug or "undefined behavior"
	// tl;dr -- if you give it the same frame (under certain circumstances), you get a seg fault

	FountainInit::init();

	fountain_decoder_sink<626> sink("/tmp");

	stringstream input = dummyContents(20000);
	fountain_encoder_stream fes = fountain_encoder_stream<626>::create(input);

	string expectedName = "pog.py";
	std::array<uint8_t,10> nameBuff = {0};
	std::copy(expectedName.begin(), expectedName.end(), nameBuff.data());
	expectedName = string(nameBuff.begin(), nameBuff.end());

	string iframe = createFrame(fes, "pog.py");
	assertEquals( 6900, iframe.size() );

	FountainMetadata md(iframe.data(), iframe.size());
	assertEquals( 20000, md.file_size() );
	assertEquals( expectedName, md.name() );

	// don't crash!
	for (int i = 0; i < 40; ++i)
		assertFalse( sink.decode_frame(iframe.data(), iframe.size()) );

	assertEquals( 1, sink.num_streams() );
	assertEquals( 0, sink.num_done() );
}
