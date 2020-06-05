#include "unittest.h"

#include "encoder/aligned_stream.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

TEST_CASE( "aligned_streamTest/testAlign", "[unit]" )
{
	/* write will look like:
	 *
	 * 0123456789
	 * 0123456789
	 * 012345----
	 * ------abcd
	 * efABCDEFGH
	 *
	 * ... we should discard the 3rd and 4th chunks.
	// */

	std::stringstream ss;
	aligned_stream aligner(ss, 10, 0);

	aligner.write("01234567890123456789012345", 26);
	assertEquals( "01234567890123456789", ss.str() );

	aligner.mark_bad_chunk(10);
	aligner.write("abcdefABCDEFGH", 14);
	assertEquals( "01234567890123456789efABCDEFGH", ss.str() );
}

TEST_CASE( "aligned_streamTest/testHeader", "[unit]" )
{
	/* write will look like:
	 *
	 * HEADER
	 * 0123456789
	 * 0123456789
	 * 012345----
	 * -abcdefABC
	 * DEFGH54321
	 *
	 * ... we should discard the 3rd and 4th chunks.
	// */

	std::stringstream ss;
	aligned_stream aligner(ss, 10, 6);

	aligner.write("HEADER01234567890123456789012345", 32);
	assertEquals( "HEADER01234567890123456789", ss.str() );

	aligner.mark_bad_chunk(5);
	aligner.write("abcdefABCDEFGH54321", 19);
	assertEquals( "HEADER01234567890123456789DEFGH54321", ss.str() );
}
