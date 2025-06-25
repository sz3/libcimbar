/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "FountainMetadata.h"
#include "fountain_encoder_stream.h"
#include "fountain_decoder_sink.h"

#include "serialize/format.h"
#include "util/File.h"
#include "util/MakeTempDirectory.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using std::string;
using namespace std;

namespace {
	stringstream dummyContents(unsigned size, string contents="0123456789")
	{
		stringstream input;
		for (unsigned i = 0; i < (size/contents.size()); ++i)
			input << contents;
		return input;
	}

	string createFrame(fountain_encoder_stream& fes)
	{
		stringstream ss;
		std::array<char, 125> buff;

		// for a 7500 byte frame, 125*60 does the trick
		for (int i = 0; i < 60; ++i)
		{
			unsigned res = fes.readsome(buff.data(), buff.size());
			assertEquals( res, buff.size() );
			ss << string(buff.data(), buff.size());
		}
		return ss.str();
	}

	std::string random_string(unsigned len, std::string chars="abcdefghijklmnaoqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890")
	{
		std::cout << ::rand() << std::endl;
		std::string output = "";
		for (unsigned i = 0; i < len; ++i)
		{
			unsigned idx = ::rand() % chars.length();
			output += chars[idx];
		}
		return output;
	}
}

TEST_CASE( "FountainSinkSpecialTest/testMultipart", "[unit]" )
{
	::srand( ::time(nullptr) );
	MakeTempDirectory tempdir;

	fountain_decoder_sink sink(750, write_on_store<std::ofstream>(tempdir.path()));

	const int totalSize = 6000000;
	string randostr = random_string(2500);
	std::cout << randostr << std::endl;
	stringstream input = dummyContents(totalSize, randostr);
	fountain_encoder_stream::ptr fes = fountain_encoder_stream::create(input, 750, 108);

	const int expectedBlocks = 1613;
	for (int i = 0; i < (totalSize / 7500) * 3; ++i)
	{
		string iframe = createFrame(*fes);
		assertEquals( 7500, iframe.size() );

		FountainMetadata md(iframe.data(), iframe.size());
		assertEquals( totalSize, md.file_size() );
		assertEquals( 108, md.encode_id() );

		if (i % 2 == 1)
		{
			int64_t res = sink.decode_frame(iframe.data(), iframe.size());
			//std::cout << i << ": " << res << std::endl;
			if (i < expectedBlocks)
			{
				assertMsg(res == 0, fmt::format("failed during  {}, res {}", i, res));
			}
			else if (i == expectedBlocks)
			{
				assertMsg(res == md.id(), fmt::format("failed {}, res {}", i, res));
				assertMsg( sink.is_done(md.id()), fmt::format("failed {}, should be done", i) );
			}
			else // i > expectedBlocks, we're already done
			{
				assertMsg(res == -1, fmt::format("failed {}, res {}", i, res));
				assertMsg( sink.is_done(md.id()), fmt::format("failed {}, should be done", i) );
			}
		}
	}

	assertEquals( 0, sink.num_streams() );
	assertEquals( 1, sink.num_done() );

	string contents = File(tempdir.path() / fmt::format("108.{}", totalSize)).read_all();
	assertEquals( totalSize, contents.size() );
	assertEquals( input.str(), contents );
}
