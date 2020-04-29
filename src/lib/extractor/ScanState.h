#pragma once

#include <deque>
#include <string>

class ScanState
{
public:
	static inline const int NOOP = -1;

public:
	ScanState();

	int process(bool active, float limit_low=3.0, float limit_high=6.0);

	std::string str() const;

protected:
	void pop_state();
	int evaluate_state(float limit_low, float limit_high);

protected:
	int _state = 0;
	std::deque<int> _tally = {0};
};
