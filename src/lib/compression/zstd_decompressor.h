#pragma once

#include "zstd/zstd.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

//template <typename STREAM>
class zstd_decompressor : public std::stringstream
{
public:
	template <typename INSTREAM>
	size_t decompress(INSTREAM& source)
	{
		// ????
		std::vector<char> srcBuff(ZSTD_DStreamInSize());
		std::vector<char> outBuff(ZSTD_DStreamOutSize());

		ZSTD_DStream* ds = ZSTD_createDStream();
		size_t initRes = ZSTD_initDStream(ds);
		if (ZSTD_isError(initRes))
		{
			_lastError << " failed init? " << ZSTD_getErrorName(initRes);
			ZSTD_freeDStream(ds);
			return 0;
		}

		size_t totalBytesRead = 0;
		bool done = false;
		while (source and !done)
		{
			source.read(srcBuff.data(), srcBuff.size());
			std::streamsize bytesRead = source.gcount();
			if (bytesRead <= 0)
			{
				_lastError << " no bytes read??? :( " << bytesRead;
				break;
			}
			totalBytesRead += bytesRead;

			ZSTD_inBuffer input = {srcBuff.data(), (size_t)bytesRead, 0};
			ZSTD_outBuffer output = {outBuff.data(), outBuff.size(), 0};

			while (input.pos < input.size)
			{
				size_t res = ZSTD_decompressStream(ds, &output, &input);
				if (ZSTD_isError(res))
				{
					_lastError << " failed decompress? " << ZSTD_getErrorName(res);
					done = true;
					break;
				}

				if (output.pos > 0)
				{
					this->write(outBuff.data(), output.pos);
					output.pos = 0;
				}
			}
		}

		ZSTD_freeDStream(ds);
		return totalBytesRead;
	}

	std::string last_error() const
	{
		return _lastError.str();
	}

protected:
	std::stringstream _lastError;
};
