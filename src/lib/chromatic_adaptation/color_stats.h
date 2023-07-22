/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "sliding_window.h"

class color_stats
{
public:
	color_stats()
	{}

	void update_low(uint8_t r, uint8_t g, uint8_t b)
	{
		_redLow.add(r);
		_greenLow.add(g);
		_blueLow.add(b);
	}

	void update_high(uint8_t r, uint8_t g, uint8_t b)
	{
		_redHigh.add(r);
		_greenHigh.add(g);
		_blueHigh.add(b);
	}

	uint8_t red_min() const
	{
		return _redHigh.min();
	}

	uint8_t red_max() const
	{
		return _redHigh.max();
	}

	uint8_t green_min() const
	{
		return _greenHigh.min();
	}

	uint8_t green_max() const
	{
		return _greenHigh.max();
	}

	uint8_t blue_min() const
	{
		return _blueHigh.min();
	}

	uint8_t blue_max() const
	{
		return _blueHigh.max();
	}

protected:
	sliding_window<uint8_t, 20> _redLow;
	sliding_window<uint8_t, 20> _redHigh;
	sliding_window<uint8_t, 20> _greenLow;
	sliding_window<uint8_t, 20> _greenHigh;
	sliding_window<uint8_t, 20> _blueLow;
	sliding_window<uint8_t, 20> _blueHigh;
};
