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

	Number nth(int n=1) const
	{
		return _nums[_nums.size()-n];
	}

	Number min() const
	{
		return _nums[0];
	}

	// count must be < N+skip
	Number mean(int count, int skip=0) const
	{
		int res = 0;
		for (int i = 1+skip; i <= count; ++i)
			res += _nums[_nums.size()-i];
		return res / count;
	}

	Number max() const
	{
		return nth(1);
	}

protected:
	std::array<Number, N> _nums;
};
