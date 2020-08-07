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
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
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
	assertStringContains("210+-24,914+-24", turbo::str::join(c3));
	assertStringContains("195+-25,61+-25", turbo::str::join(c3));
	assertStringContains("1039+-22,887+-23", turbo::str::join(c3));

	std::vector<Anchor> c4;
	for (const Anchor& c : c3)
		sc.t4_confirm_scan<ScanState_114>(c, [&c4] (const Anchor& p) { c4.push_back(p); });

	candidates = sc.deduplicate_candidates(c4);
	sc.filter_candidates(candidates);

	// ordered by size
	assertEquals(
	    "195+-25,61+-25 210+-24,914+-24 1039+-22,887+-23",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 1730, cutoff );
	assertEquals(
	    "210+-24,914+-24 195+-25,61+-25 1039+-22,887+-23",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "210+-24,914+-24 195+-25,61+-25 1039+-22,887+-23 1036+-23,67+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testBottomRightCorner.2", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4color_ecc30_fountain_0.png"));
	Scanner sc(img);

	std::vector<Anchor> candidates;
	int cutoff = sc.scan_primary(candidates);
	assertEquals( 2255, cutoff );
	assertEquals(
	    "29+-27,29+-27 994+-27,29+-27 29+-27,993+-27",
	    turbo::str::join(candidates)
	);

	assertTrue( sc.add_bottom_right_corner(candidates, cutoff) );
	assertEquals(
	    "29+-27,29+-27 994+-27,29+-27 29+-27,993+-27 993+-27,994+-28",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "210+-24,914+-24 195+-25,61+-25 1039+-22,887+-23 1036+-23,67+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.2", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f1_360.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "41+-25,196+-25 909+-24,183+-26 69+-23,1036+-23 896+-23,1036+-24",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testExampleScan.3", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4color_ecc30_fountain_0.png"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right

	assertEquals(
	    "29+-27,29+-27 994+-27,29+-27 29+-27,993+-27 993+-27,994+-28",
	    turbo::str::join(candidates)
	);
}


TEST_CASE( "ScannerTest/testScanEdges", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates = sc.scan();
	// order is top-left, top-right, bottom-left, bottom-right
	assertEquals(
	    "210+-24,914+-24 195+-25,61+-25 1039+-22,887+-23 1036+-23,67+-24",
	    turbo::str::join(candidates)
	);

	Corners cs(candidates);
	Midpoints mp;
	std::vector<point<int>> edges = sc.scan_edges(cs, mp);
	assertEquals( "177,490 624,38 1062,479 633,925", turbo::str::join(edges) );

	// check "expected" midpoints as well.
	assertEquals( "202.553,490.534 623.89,64.0599 1037.51,479.803 632.655,900.234", turbo::str::join(mp.points()) );
}

TEST_CASE( "ScannerTest/testSortTopToBottom", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates;
	candidates.push_back(Anchor(300, 360, 100, 160));
	candidates.push_back(Anchor(300, 360, 300, 360));
	candidates.push_back(Anchor(100, 160, 300, 360));
	assertTrue( sc.sort_top_to_bottom(candidates) ); // make this a static function?

	assertEquals(
	    "330+-30,330+-30 330+-30,130+-30 130+-30,330+-30",
	    turbo::str::join(candidates)
	);
}

TEST_CASE( "ScannerTest/testSortTopToBottom.2", "[unit]" )
{
	cv::Mat img = cv::imread(TestCimbar::getSample("4_30_f0_627.jpg"));
	Scanner sc(img);

	std::vector<Anchor> candidates;
	candidates.push_back(Anchor(966, 1020, 966, 1020));
	candidates.push_back(Anchor(966, 1020, 2, 56));
	candidates.push_back(Anchor(2, 56, 966, 1020));
	assertTrue( sc.sort_top_to_bottom(candidates) );

	assertEquals(
	    "993+-27,993+-27 29+-27,993+-27 993+-27,29+-27",
	    turbo::str::join(candidates)
	);
}
