/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

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
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f0_627.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates;
	sc.t1_scan_rows<ScanState_114>([&candidates] (const Anchor& p) { candidates.push_back(p); });
	std::string res = turbo::str::join(candidates);
	assertStringContains("210+-25,912+-0", res);
	assertStringContains("195+-25,64+-0", res);
	assertStringContains("1039+-23,880+-0", res);

	std::vector<Anchor> c2;
	for (const Anchor& c : candidates)
		sc.t2_scan_column<ScanState_114>(c, [&c2] (const Anchor& p) { c2.push_back(p); });
	assertStringContains("210+-0,914+-24", turbo::str::join(c2));
	assertStringContains("195+-0,61+-25", turbo::str::join(c2));
	assertStringContains("1039+-0,887+-23", turbo::str::join(c2));

	std::vector<Anchor> c3;
	for (const Anchor& c : c2)
		sc.t3_scan_diagonal<ScanState_114>(c, [&c3] (const Anchor& p) { c3.push_back(p); });
	assertStringContains("210+-23,914+-24", turbo::str::join(c3));
	assertStringContains("195+-25,61+-25", turbo::str::join(c3));
	assertStringContains("1039+-22,887+-23", turbo::str::join(c3));

	std::vector<Anchor> c4;
	for (const Anchor& c : c3)
		sc.t4_confirm_scan<ScanState_114>(c, true, [&c4] (const Anchor& p) { c4.push_back(p); });

	candidates = sc.deduplicate_candidates(c4);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "195+-25,61+-25 210+-25,914+-24 1039+-23,887+-23",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f0_627.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 1766, cutoff );
	assertEquals(
	    "210+-25,914+-24 195+-25,61+-25 1039+-23,887+-23",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "210+-25,914+-24 195+-25,61+-25 1039+-23,887+-23 1035+-23,68+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner.2", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 2268, cutoff );
	assertEquals(
	    "29+-27,29+-27 993+-27,29+-27 29+-27,993+-27",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "29+-27,29+-27 993+-27,29+-27 29+-27,993+-27 993+-27,993+-27",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner.3", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f2_734.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 1575, cutoff );
	assertEquals(
	    "56+-24,166+-25 870+-24,133+-25 137+-20,910+-18",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "56+-24,166+-25 870+-24,133+-25 137+-20,910+-18 837+-18,897+-18",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner.4", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f2_246.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 1606, cutoff );
	assertEquals(
	    "189+-25,899+-23 157+-26,79+-25 924+-18,811+-19",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "189+-25,899+-23 157+-26,79+-25 924+-18,811+-19 911+-18,122+-19",
	    turbo::str::join(candidates)
	);
}


TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f0_627.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "210+-25,914+-24 195+-25,61+-25 1039+-23,887+-23 1035+-23,68+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.2", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f1_360.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "41+-24,196+-25 909+-25,183+-25 69+-23,1036+-23 896+-23,1036+-23",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.3", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4color_ecc30_fountain_0.png");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "29+-27,29+-27 993+-27,29+-27 29+-27,993+-27 993+-27,993+-27",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.Adaptive", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f0_627.jpg");
	Scanner sc(img, false);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "210+-25,914+-24 195+-25,61+-25 1039+-23,887+-23 1035+-23,67+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testScanEdges", "[unit]" )
{
	cv::Mat img = TestCimbar::loadSample("6bit/4_30_f0_627.jpg");
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "210+-25,914+-24 195+-25,61+-25 1039+-23,887+-23 1035+-23,68+-24",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	Midpoints mp;
	std::vector<point<int>> edges = sc.scan_edges(cs, mp);
	assertEquals( "177,490 623,39 1061,479 633,925", turbo::str::join(edges) );

	// check "expected" midpoints as well.
	assertEquals( "202.549,490.268 623.627,64.5719 1037.01,480.052 632.907,900.226", turbo::str::join(mp.points()) );
}

TEST_CASE( "ScannerTest/testSortTopToBottom", "[unit]" )
{
	std::vector<Anchor> candidates;
	candidates.push_back(Anchor(300, 360, 100, 160));
	candidates.push_back(Anchor(300, 360, 300, 360));
	candidates.push_back(Anchor(100, 160, 300, 360));
	assertTrue( Scanner::sort_top_to_bottom(candidates) );

	assertEquals(
		"330+-30,330+-30 130+-30,330+-30 330+-30,130+-30",
		turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSortTopToBottom.2", "[unit]" )
{
	std::vector<Anchor> candidates;
	candidates.push_back(Anchor(966, 1020, 966, 1020));
	candidates.push_back(Anchor(966, 1020, 2, 56));
	candidates.push_back(Anchor(2, 56, 966, 1020));
	assertTrue( Scanner::sort_top_to_bottom(candidates) );

	assertEquals(
	    "993+-27,993+-27 29+-27,993+-27 993+-27,29+-27",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSortTopToBottom.3", "[unit]" )
{
	std::vector<Anchor> candidates;
	candidates.push_back(Anchor(383, 437, 994, 1048));
	candidates.push_back(Anchor(395, 447, 107, 157));
	candidates.push_back(Anchor(1250, 1296, 124, 170));
	assertTrue( Scanner::sort_top_to_bottom(candidates) );

	assertEquals(
		"421+-26,132+-25 1273+-23,147+-23 410+-27,1021+-27",
		turbo::str::join(candidates)
	);
}
