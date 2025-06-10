/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <ctime>

class TimeAccumulator
{
public:
	TimeAccumulator()
	{}

	void increment(clock_t num)
	{
		_total += num;
		_ticks += 1;
	}

	double avg() const
	{
		if (_ticks == 0)
			return 0;
		return _total/_ticks;
	}

	clock_t ticks() const
	{
		return _ticks;
	}

protected:
	clock_t _total = 0;
	clock_t _ticks = 0;
};

class Timer
{
public:
	Timer(TimeAccumulator& accum)
		: _accum(accum)
		, _start(clock())
	{}

	~Timer()
	{
		_accum.increment(clock() - _start);
	}

protected:
	TimeAccumulator& _accum;
	clock_t _start;
};
