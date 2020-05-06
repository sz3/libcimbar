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
