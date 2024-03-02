/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

class null_stream
{
public:
	null_stream()
	{}

	null_stream& write(const char*, unsigned length)
	{
		_count += length;
		return *this;
	}

	bool good() const
	{
		return true;
	}

	long tellp() const
	{
		return _count;
	}

protected:
	long _count = 0;
};
