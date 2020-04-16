#pragma once

#include <deque>

class ScanState
{
public:
	static inline const int NOOP = -1;

public:
	ScanState();

	int process(bool active, float leniency=3.0);

protected:
	void pop_state();
	int evaluate_state(float leniency);

protected:
	int _state = 0;
	std::deque<int> _tally = {0};
};
