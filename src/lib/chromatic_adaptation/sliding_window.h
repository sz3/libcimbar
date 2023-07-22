/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <deque>

// obviously this is not an optimized implementation
template <typename T, int SIZE=10>
class sliding_window
{
public:
	sliding_window()
	{
		for (int i = 0; i < SIZE; ++i)
			_store.push_back(150);
	}

	void add(const T& val)
	{
		_store.pop_front();
		_store.push_back(val);
	}

	T mean() const
	{
		int res = 0;
		for (const T& elem : _store)
			res += elem;
		return res / SIZE;
	}

	T min() const
	{
		return *std::min_element(_store.begin(), _store.end());
	}

	T max() const
	{
		return *std::max_element(_store.begin(), _store.end());
	}

protected:
	std::deque<T> _store;
	bool _active;
};
