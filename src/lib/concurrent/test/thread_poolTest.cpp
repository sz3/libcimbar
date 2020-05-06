/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"

#include "thread_pool.h"

#include "concurrent/monitor.h"
#include "serialize/str_join.h"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>
using std::lock_guard;
using std::mutex;
using turbo::monitor;

TEST_CASE( "thread_poolTest/testDefault", "[unit]" )
{
	turbo::thread_pool threads(5);
	assertTrue( threads.start() );

	mutex myMutex;
	std::vector<unsigned> results;
	for (unsigned i = 0; i < 10; ++i)
		threads.execute( [&, i] () { lock_guard<mutex> lock(myMutex); results.push_back(i); } );

	monitor m;
	threads.execute( [&m] () { m.signal_all(); } );
	assertTrue( m.wait_for(1000) );

	while (results.size() < 10)
		std::cout << threads.queued() << std::endl;

	std::sort(results.begin(), results.end());
	assertEquals( "0 1 2 3 4 5 6 7 8 9", turbo::str::join(results) );
}


TEST_CASE( "thread_poolTest/testTryExecute", "[unit]" )
{
	turbo::thread_pool threads(3, 1);

	mutex myMutex;
	std::vector<unsigned> results;

	// fill up the queue
	for (unsigned i = 0; i < 64; ++i)
		assertTrue( threads.try_execute( [&, i] () { lock_guard<mutex> lock(myMutex); results.push_back(i); } ) );

	// can't schedule any more
	for (unsigned i = 64; i < 70; ++i)
		assertFalse( threads.try_execute( [&, i] () { lock_guard<mutex> lock(myMutex); results.push_back(i); } ) );

	assertTrue( threads.start() );
	monitor m;
	threads.execute( [&m] () { m.signal_all(); } );
	assertTrue( m.wait_for(1000) );

	while (results.size() < 10)
		std::cout << threads.queued() << std::endl;

	std::sort(results.begin(), results.end());
	assertStringContains( "0 1 2 3 4 5 6 7 8 9", turbo::str::join(results) );
	assertEquals( 64, results.size() );
}

