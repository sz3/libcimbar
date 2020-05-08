#pragma once

#include "wirehair/wirehair.h"

namespace WireHairInit {
	static bool init()
	{
		static WirehairResult res = wirehair_init();
		return res;
	}
}
