/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <ctime>

class Timer
{
public:
	Timer(clock_t& total)
		: _total(total)
		, _start(clock())
	{}

	~Timer()
	{
		_total += clock() - _start;
	}

protected:
	clock_t& _total;
	clock_t _start;
};
