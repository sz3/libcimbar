/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "cimb_translator/Config.h"

class ConfigScope : cimbar::Config
{
public:
	using cimbar::Config::active_conf;

	ConfigScope(int mode_val=0)
	{
		cimbar::Config::update(mode_val);
	}

	~ConfigScope()
	{
		// reset
		cimbar::Config::update();
	}

protected:
};
