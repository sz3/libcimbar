/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "zstd/zstd.h"
#include <sstream>
#include <string>

namespace cimbar {

class zstd_dstream
{
public:
	zstd_dstream()
	{
		_ds = ZSTD_createDStream();
		if (!_ds)
		{
			_lastError << "failed ZSTD_createDStream!";
			return;
		}

		size_t initRes = ZSTD_initDStream(_ds);
		if (ZSTD_isError(initRes))
		{
			_lastError << " failed init? " << ZSTD_getErrorName(initRes);
			ZSTD_freeDStream(_ds);
			_ds = nullptr;
		}

	}

	~zstd_dstream()
	{
		if (_ds)
			ZSTD_freeDStream(_ds);
	}

	operator bool() const
	{
		return _ds;
	}

	operator ZSTD_DStream*()
	{
		return _ds;
	}

	std::string last_error() const
	{
		return _lastError.str();
	}

protected:
	ZSTD_DStream* _ds;
	std::stringstream _lastError;
};

}
