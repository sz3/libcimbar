#pragma once

#include "wirehair/wirehair.h"

namespace FountainInit {
	static bool init()
	{
		static WirehairResult res = wirehair_init();
		return res;
	}
}
