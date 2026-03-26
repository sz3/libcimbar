/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "zstd_dstream.h"

#include "zstd/zstd.h"
#include <array>
#include <sstream>
#include <vector>

namespace cimbar {

template <typename STREAM>
class zstd_decompressor : public STREAM
{
public:
	using STREAM::STREAM; // pull in constructors

public:
	bool write_once()
	{
		size_t writeLen = CHUNK_SIZE;
		while (_inBuff.size() > 0)
		{
			if (_inBuff.size() < CHUNK_SIZE)
				writeLen = _inBuff.size();

			ZSTD_inBuffer input = {_inBuff.data(), writeLen, 0};
			ZSTD_outBuffer output = {_outBuff.data(), _outBuff.size(), 0};

			while (input.pos < input.size)
			{
				size_t res = ZSTD_decompressStream(_ds, &output, &input);
				if (ZSTD_isError(res))
				{
					_lastError << " failed decompress? " << ZSTD_getErrorName(res);
					return false;
				}

				if (output.pos > 0)
				{
					STREAM::write(_outBuff.data(), output.pos);
					_inBuff = std::string_view(_inBuff.data() + input.pos, _inBuff.size() - input.pos);
					return true;
				}
			}
			_inBuff = std::string_view(_inBuff.data() + input.size, _inBuff.size() - input.size);
		}
		return false;
	}

	bool init_decompress(const char* data, size_t len)
	{
		if (!_ds)
			return false;
		_inBuff = std::string_view(data, len);
		return true;
	}

	bool write(const char* data, size_t len)
	{
		if (!init_decompress(data, len))
			return false;
		while (_inBuff.size() > 0)
			write_once();
		return true;
	}

	template <typename INSTREAM>
	size_t decompress(INSTREAM& source)
	{
		if (!_ds)
			return 0;

		std::vector<char> srcBuff(CHUNK_SIZE);
		size_t totalBytesRead = 0;
		while (source)
		{
			source.read(srcBuff.data(), srcBuff.size());
			std::streamsize bytesRead = source.gcount();
			if (bytesRead <= 0)
			{
				_lastError << " no bytes read??? :( " << bytesRead;
				break;
			}
			totalBytesRead += bytesRead;

			if (!write(srcBuff.data(), bytesRead))
				break;
		}
		return totalBytesRead;
	}

	std::string last_error() const
	{
		return _lastError.str();
	}	

protected:
	const size_t CHUNK_SIZE = ZSTD_DStreamInSize();
	zstd_dstream _ds;
	std::vector<char> _outBuff = std::vector<char>(ZSTD_DStreamOutSize());
	std::string_view _inBuff;

	std::stringstream _lastError;
};

}
