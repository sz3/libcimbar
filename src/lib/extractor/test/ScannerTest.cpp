#include "unittest.h"

#include "Scanner.h"

#include "Corners.h"
#include "Midpoints.h"
#include "Point.h"
#include "serialize/str_join.h"
#include <iostream>
#include <string>
#include <vector>


TEST_CASE( "ScannerTest/testPiecemealScan", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.t1_scan_rows();
	std::string res = turbo::str::join(candidates);
	assertStringContains("51+-26,289+-0", res);
	assertStringContains("992+-27,289+-0", res);
	assertStringContains("85+-24,1190+-0", res);
	assertStringContains("959+-25,1190+-0", res);

	candidates = sc.t2_scan_columns(candidates);
	assertStringContains("51+-27,286+-28", turbo::str::join(candidates));
	assertStringContains("993+-27,283+-28", turbo::str::join(candidates));
	assertStringContains("85+-24,1188+-24", turbo::str::join(candidates));
	assertStringContains("958+-25,1196+-24", turbo::str::join(candidates));

	candidates = sc.t3_scan_diagonal(candidates);
	assertStringContains("52+-28,287+-28", turbo::str::join(candidates));
	assertStringContains("993+-27,283+-27", turbo::str::join(candidates));
	assertStringContains("85+-24,1188+-24", turbo::str::join(candidates));
	assertStringContains("959+-24,1197+-24", turbo::str::join(candidates));

	candidates = sc.t4_confirm_scan(candidates);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "52+-28,286+-28 992+-27,283+-28 85+-24,1188+-24 959+-25,1196+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "52+-28,286+-28 "
	    "992+-27,283+-28 "
	    "85+-24,1188+-24 "
	    "959+-25,1196+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testScanEdges", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4c-cam-40-f1.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "52+-28,286+-28 "
	    "992+-27,283+-28 "
	    "85+-24,1188+-24 "
	    "959+-25,1196+-24",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	Midpoints mp;
	std::vector<point<int>> edges = sc.scan_edges(cs, mp);
	assertEquals( "518,255 1004,757 519,1219 41,754", turbo::str::join(edges) );

	// check "expected" midpoints as well.
	assertEquals( "518.995,284.51 974.896,756.212 519.402,1191.98 69.0967,753.309", turbo::str::join(mp.points()) );
}

