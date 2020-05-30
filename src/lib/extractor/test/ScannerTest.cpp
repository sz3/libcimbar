#include "unittest.h"

#include "Scanner.h"
#include "serialize/str_join.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
	std::string get_sample(std::string filename)
	{
		return std::string(LIBCIMBAR_PROJECT_ROOT) + "/samples/" + filename;
	}
}

TEST_CASE( "ScannerTest/testPiecemealScan", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.t1_scan_rows();
	std::string res = turbo::str::join(candidates);
	assertStringContains("x=106-229,y=952-952", res);
	assertStringContains("x=110-233,y=1003-1003", res);
	assertStringContains("x=2339-2478,y=884-884", res);
	assertStringContains("x=2339-2477,y=918-918", res);
	assertStringContains("x=2282-2405,y=3043-3043", res);
	assertStringContains("x=2281-2403,y=3077-3077", res);
	assertStringContains("x=267-377,y=3009-3009", res);
	assertStringContains("x=271-380,y=3043-3043", res);

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("x=170-170,y=915-1047", turbo::str::join(candidates));
	assertStringContains("x=2408-2408,y=826-967", turbo::str::join(candidates));
	assertStringContains("x=2343-2343,y=3010-3122", turbo::str::join(candidates));
	assertStringContains("x=325-325,y=2977-3084", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	assertStringContains("x=106-234,y=917-1045", turbo::str::join(candidates));
	assertStringContains("x=2342-2476,y=830-964", turbo::str::join(candidates));
	assertStringContains("x=2285-2399,y=3009-3123", turbo::str::join(candidates));
	assertStringContains("x=268-378,y=2976-3086", turbo::str::join(candidates));

	candidates = sc.t4_confirm_scan(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "x=2342-2476,y=830-964 "
	    "x=104-235,y=917-1045 "
	    "x=268-381,y=2976-3086 "
	    "x=2285-2400,y=3009-3123",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "x=104-235,y=917-1045 "
	    "x=2342-2476,y=830-964 "
	    "x=268-381,y=2976-3086 "
	    "x=2285-2400,y=3009-3123",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSmallSample.1", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("smol70.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "x=43-102,y=44-102 "
	    "x=912-956,y=56-100 "
	    "x=126-167,y=901-942 "
	    "x=892-928,y=840-876",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSmallSample.2", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("smol110.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	// x=58-114,y=100-156 x=914-959,y=106-151 x=673-686,y=581-594 x=904-942,y=888-926
	assertEquals(
	    "x=58-114,y=101-156 "
	    "x=914-959,y=106-151 "
	    "x=130-172,y=949-991 "
	    "x=904-941,y=888-925",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSmallSample.3", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color-cam-140.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals("33+-25,124+-25 819+-23,132+-25 111+-17,827+-17 747+-18,839+-16", turbo::str::join(candidates));
}

TEST_CASE( "ScannerTest/testSmallSample.4", "[unit]" )
{
	cv::Mat img = cv::imread(get_sample("4color-cam-200.png"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals("37+-25,103+-25 822+-22,117+-25 113+-17,810+-17 746+-18,823+-16", turbo::str::join(candidates));
}
