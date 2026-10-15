/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "zstd_dstream.h"

#include "zstd/zstd.h"
#include <array>
#include <cstdint>
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
					_isError = true;
					return false; // we're not making progress, mayday mayday
				}

				if (output.pos > 0)
				{
					STREAM::write(_outBuff.data(), output.pos);
					_inBuff = std::string_view(_inBuff.data() + input.pos, _inBuff.size() - input.pos);

					_dstByteCount += output.pos;
					_srcByteCount += input.pos;

					//std::cerr << "src bytes " << _srcByteCount << ", dst bytes " << _dstByteCount << std::endl;
					// if _byteCount too large, fail?
					if (_dstByteCount > 50'000'000 and (_srcByteCount << 8) < _dstByteCount) // *256
					{
						_lastError << " detected suspicious compression ratio. Is someone up to no good?";
						_isError = true;
						return false;
					}
					return true;
				}
			}
			_inBuff = std::string_view(_inBuff.data() + input.size, _inBuff.size() - input.size);
		}
		return true;
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
			if (!write_once())
				return false;
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

	bool good() const
	{
		return !_isError and STREAM::good();
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

	bool _isError = false;
	std::stringstream _lastError;

	uint64_t _srcByteCount = 0;
	uint64_t _dstByteCount = 0;
};

}
