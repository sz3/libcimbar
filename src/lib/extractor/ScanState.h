#pragma once

#include "serialize/str_join.h"
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

inline ScanState::ScanState()
{}

inline void ScanState::pop_state()
{
	_state -= 2;
	_tally.pop_front();
	_tally.pop_front();
}

inline int ScanState::evaluate_state(float limit_low, float limit_high)
{
	if (_state != 6)
		return NOOP;

	for (int i = 1; i <= 5; ++i)
		if (_tally[i] == 0)
			return NOOP;

	int center = _tally[3];
	for (int i = 1; i <= 5; ++i)
	{
		if (i == 3)
			continue;
		float ratio_min = center / (_tally[i] + 1);
		float ratio_max = center / std::max(1, _tally[i] - 1);
		if (ratio_max < limit_low or ratio_min > limit_high)
			return NOOP;
	}

	int size = 0;
	for (int i = 1; i <= 5; ++i)
		size += _tally[i];
	return size;
}

inline int ScanState::process(bool active, float limit_low, float limit_high)
{
	bool isTransition = false;
	bool even = _state == 0 or _state == 2 or _state == 4; // not 6
	bool odd = _state == 1 or _state == 3 or _state == 5;
	if ((even and active) or (odd and !active))
		isTransition = true;

	if (isTransition)
	{
		_state += 1;
		_tally.push_back(1);
		if (_state == 6)
		{
			int res = evaluate_state(limit_low, limit_high);
			pop_state();
			return res;
		}
		return NOOP;
	}

	// else !isTransition
	if (odd and active)
		_tally.back() += 1;
	if (!active and (_state == 2 or _state == 4))
		_tally.back() += 1;
	return NOOP;
}

inline std::string ScanState::str() const
{
	return turbo::str::join(_tally);
}
