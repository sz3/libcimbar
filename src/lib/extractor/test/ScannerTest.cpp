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
	assertStringContains("x=106-233,y=952-1003", turbo::str::join(candidates));
	assertStringContains("x=2338-2478,y=867-918", turbo::str::join(candidates));
	assertStringContains("x=2281-2405,y=3043-3077", turbo::str::join(candidates));
	assertStringContains("x=268-380,y=3009-3043", turbo::str::join(candidates));

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("x=169-169,y=915-1047", turbo::str::join(candidates));
	assertStringContains("x=2408-2408,y=826-967", turbo::str::join(candidates));
	assertStringContains("x=2343-2343,y=3010-3122", turbo::str::join(candidates));
	assertStringContains("x=324-324,y=2977-3083", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	// order is arbitrary (but consistent)
	assertEquals(
	    "x=2343-2476,y=831-964 "
	    "x=106-233,y=918-1045 "
	    "x=270-379,y=2976-3085 "
	    "x=2286-2400,y=3009-3123",
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
	    "x=106-233,y=918-1045 "
	    "x=2343-2476,y=831-964 "
	    "x=270-379,y=2976-3085 "
	    "x=2286-2400,y=3009-3123",
	    turbo::str::join(candidates)
	);
}
