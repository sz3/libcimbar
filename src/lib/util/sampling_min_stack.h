/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <array>
#include <cstdint>

template <typename Integer=uint16_t>
class sampling_min_stack
{
public:
	sampling_min_stack()
	    : _vals{255, 255, 255, 255}
	    , _minSum(255*4)
	    , _totalSum(-_minSum)
	{}

	Integer min_sum() const
	{
		return _minSum;
	}

	Integer avg() const
	{
		return _minSum >> 2; // /4, the length of _vals
	}

	Integer total() const
	{
		return _totalSum;
	}

	// replace max in _vals with val. Returns previous max value.
	// unconditional -- assumes val < avg(), or none of this makes sense
	Integer push_pop(Integer replacement)
	{
		// get max idx
		int8_t il = _vals[0] > _vals[1]? 0 : 1;
		int8_t ir = _vals[2] > _vals[3]? 2 : 3;
		int8_t i = _vals[il] > _vals[ir]? il : ir;

		Integer res = _vals[i];
		_vals[i] = replacement;
		_minSum += replacement - res;
		return res;
	}

	void add(Integer prospect)
	{
		if (prospect < avg())
			prospect = push_pop(prospect);
		_totalSum += prospect;
	}

	sampling_min_stack& operator+=(Integer prospect)
	{
		add(prospect);
		return *this;
	}

protected:
	std::array<Integer, 4> _vals;
	Integer _minSum;
	Integer _totalSum;
};
