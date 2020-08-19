/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <deque>

class EdgeScanState
{
public:
	static inline const int NOOP = -1;

public:
	EdgeScanState() {}

	int process(bool active)
	{
		bool is_transition = (_state == 0 and active) or (_state == 1 and !active);
		if (is_transition)
		{
			_state += 1;
			_tally.push_back(0);
			_tally.back() += 1;

			if (_state == 2)
			{
				int res = _tally[1];
				pop_state();
				return res;
			}
			return NOOP;
		}

		if (_state == 1 and active)
			_tally.back() += 1;

		if (_state == 0 and !active)
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

protected:
	int _state = 0;
	std::deque<int> _tally = {0};
};
