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

	string createFrame(string name, unsigned size)
	{
		stringstream input = dummyContents(size);
		fountain_encoder_stream fes = fountain_encoder_stream<599>::create(input);
		fes.encode_metadata_block(name);

		stringstream ss;
		std::array<char, 140> buff;

		// for a 8400 byte frame, 140*60 does the trick
		// 599*14 + 14 byte md block
		for (int i = 0; i < 60; ++i)
		{
			unsigned res = fes.readsome(buff.data(), buff.size());
			assertEquals( res, buff.size() );
			ss << string(buff.data(), buff.size());
		}
		return ss.str();
	}
}

TEST_CASE( "FountainSinkTest/testDefault", "[unit]" )
{
	FountainInit::init();

	fountain_decoder_sink<599> sink("/tmp");
	string iframe = createFrame("theBeginning", 1200);
	assertEquals( 8400, iframe.size() );

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
