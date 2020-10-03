/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

template <typename CONTAINER, typename CONTAINER_IT=typename CONTAINER::const_iterator>
class loop_iterator
{
public:
	loop_iterator(const CONTAINER& c)
	    : _c(c)
	    , _it(c.begin())
	{
	}

	operator bool() const
	{
		return _it != _c.end();
	}

	operator CONTAINER_IT() const
	{
		return _it;
	}

	loop_iterator& operator++()
	{
		++_it;
		if (_it == _c.end() and !_halt)
			reset();
		return *this;
	}

	CONTAINER_IT end() const
	{
		return _c.end();
	}

	void reset()
	{
		_it = _c.begin();
	}

	void halt()
	{
		_halt = true;
	}

protected:
	const CONTAINER& _c;
	CONTAINER_IT _it;
	bool _halt = false;
};
