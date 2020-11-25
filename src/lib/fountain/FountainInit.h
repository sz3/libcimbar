/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "wirehair/wirehair.h"

namespace FountainInit {
	static bool init()
	{
		static WirehairResult res = wirehair_init();
		return !res;
	}
}
