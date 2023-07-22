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

protected:
	sliding_window<uint8_t> _redLow;
	sliding_window<uint8_t> _redHigh;
	sliding_window<uint8_t> _greenLow;
	sliding_window<uint8_t> _greenHigh;
	sliding_window<uint8_t> _blueLow;
	sliding_window<uint8_t> _blueHigh;
};
