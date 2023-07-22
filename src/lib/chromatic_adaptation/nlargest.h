/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <algorithm>
#include <array>
#include <iostream>

template <typename Number, int N>
class nlargest
{
public:
	nlargest()
		: _nums({})
	{}

	template <typename T>
	void eval(T&& num)
	{
		if (num > min())
		{
			_nums[0] = num;
			std::sort(_nums.begin(), _nums.end());
		}
	}

	Number min() const
	{
		return _nums[0];
	}

	Number max() const
	{
		return _nums[N-1];
	}

protected:
	std::array<Number, N> _nums;
};
