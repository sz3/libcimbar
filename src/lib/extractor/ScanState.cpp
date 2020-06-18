#include "ScanState.h"

#include "serialize/str_join.h"

ScanState::ScanState() {}

void ScanState::pop_state()
{
	_state -= 2;
	_tally.pop_front();
	_tally.pop_front();
}

int ScanState::evaluate_state(float limit_low, float limit_high)
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

int ScanState::process(bool active, float limit_low, float limit_high)
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

std::string ScanState::str() const
{
	return turbo::str::join(_tally);
}
