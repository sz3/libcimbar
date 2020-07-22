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

	string createFrame(fountain_encoder_stream& fes)
	{
		stringstream ss;
		std::array<char, 115> buff;

		// for a 6900 byte frame, 115*60 does the trick
		for (int i = 0; i < 60; ++i)
		{
			unsigned res = fes.readsome(buff.data(), buff.size());
			assertEquals( res, buff.size() );
			ss << string(buff.data(), buff.size());
		}
		return ss.str();
	}

	string createFrame(uint8_t encode_id, unsigned size)
	{
		stringstream input = dummyContents(size);
		fountain_encoder_stream fes = fountain_encoder_stream::create(input, 690, encode_id);
		return createFrame(fes);
	}
}

TEST_CASE( "FountainSinkTest/testDefault", "[unit]" )
{
	FountainInit::init();

	fountain_decoder_sink sink("/tmp", 690);
	string iframe = createFrame(0, 1200);
	assertEquals( 6900, iframe.size() );

	FountainMetadata md(iframe.data(), iframe.size());
	assertEquals( 1200, md.file_size() );
	assertEquals( 0, md.encode_id() );

	assertEquals( true, sink.decode_frame(iframe.data(), iframe.size()) );
	assertEquals( true, sink.is_done(md.id()) );
	assertEquals( false, sink.decode_frame(iframe.data(), iframe.size()) );

	string frame2 = createFrame(1, 1600);
	assertEquals( true, sink.decode_frame(frame2.data(), frame2.size()) );
	assertEquals( true, sink.is_done(FountainMetadata(1, 1600).id()) );

	assertEquals( 0, sink.num_streams() );
	assertEquals( 2, sink.num_done() );

	string contents = File("/tmp/0.1200").read_all();
	assertEquals( 1200, contents.size() );
	contents = File("/tmp/1.1600").read_all();
	assertEquals( 1600, contents.size() );
}

TEST_CASE( "FountainSinkTest/testMultipart", "[unit]" )
{
	FountainInit::init();

	fountain_decoder_sink sink("/tmp", 690);

	stringstream input = dummyContents(20000);
	fountain_encoder_stream fes = fountain_encoder_stream::create(input, 690, 2);

	for (int i = 0; i < 4; ++i)
	{
		string iframe = createFrame(fes);
		assertEquals( 6900, iframe.size() );

		FountainMetadata md(iframe.data(), iframe.size());
		assertEquals( 20000, md.file_size() );
		assertEquals( 2, md.encode_id() );

		assertMsg( ((i == 2) == sink.decode_frame(iframe.data(), iframe.size())), fmt::format("failed {}", i) );
		assertMsg( ((i >= 2) == sink.is_done(md.id())), fmt::format("failed {}", i) );
	}

	assertEquals( 0, sink.num_streams() );
	assertEquals( 1, sink.num_done() );

	string contents = File("/tmp/2.20000").read_all();
	assertEquals( 20000, contents.size() );
}

TEST_CASE( "FountainSinkTest/testSameFrameManyTimes", "[unit]" )
{
	// if you give wirehair the same frame (under certain circumstances), you get a seg fault
	// sometimes it's fine. The docs say "don't do it", so FountainDecoder acts as the bouncer.
	FountainInit::init();

	fountain_decoder_sink sink("/tmp", 690);

	stringstream input = dummyContents(20000);
	fountain_encoder_stream fes = fountain_encoder_stream::create(input, 690, 3);

	string iframe = createFrame(fes);
	assertEquals( 6900, iframe.size() );

	FountainMetadata md(iframe.data(), iframe.size());
	assertEquals( 20000, md.file_size() );
	assertEquals( 3, md.encode_id() );

	// don't crash!
	for (int i = 0; i < 40; ++i)
		assertFalse( sink.decode_frame(iframe.data(), iframe.size()) );

	assertEquals( 1, sink.num_streams() );
	assertEquals( 0, sink.num_done() );
}
