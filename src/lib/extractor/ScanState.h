/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <deque>
#include <string>
#include <vector>

class ScanState
{
public:
	static inline const int NOOP = -1;

	using LimitList = std::vector<std::pair<float, float>>;

protected:
	ScanState(const LimitList& limits)
	    : _limits(limits)
	{}

public:
	int process(bool active)
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
				int res = evaluate_state();
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

protected:
	void pop_state()
	{
		_state -= 2;
		_tally.pop_front();
		_tally.pop_front();
	}

	int evaluate_state()
	{
		if (_state != 6)
			return NOOP;

		for (int i = 1; i <= 5; ++i)
			if (_tally[i] == 0)
				return NOOP;

		float center = _tally[3];
		for (int i = 1; i <= 5; ++i)
		{
			if (i == 3)
				continue;
			float ratio_min = center / (_tally[i] + 1);
			float ratio_max = center / std::max(1, _tally[i] - 1);
			if (ratio_max < _limits[i].first or ratio_min > _limits[i].second)
				return NOOP;
		}

		int size = 0;
		for (int i = 1; i <= 5; ++i)
			size += _tally[i];
		return size;
	}

protected:
	int _state = 0;
	std::deque<int> _tally = {0};
	LimitList _limits;
};

class ScanState_114 : public ScanState
{
public:
	ScanState_114()
	    : ScanState({{0,0}, {3.0, 6.0}, {3.0, 6.0}, {0,0}, {3.0, 6.0}, {3.0, 6.0}})
	{}
};

class ScanState_122 : public ScanState
{
public:
	ScanState_122()
	    : ScanState({{0,0}, {1.0, 3.0}, {0.5, 1.5}, {0,0}, {0.5, 1.5}, {1.0, 3.0}})
	{}
};
