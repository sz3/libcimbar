/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "zstd/zstd.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

namespace cimbar {

template <typename STREAM>
class zstd_compressor : public STREAM
{
public:
	using STREAM::STREAM; // pull in constructors

public:
	~zstd_compressor()
	{
		if (_cctx)
			ZSTD_freeCCtx(_cctx);
	}

	bool write(const char* data, size_t len)
	{
		size_t writeLen = CHUNK_SIZE;
		while (len > 0)
		{
			if (len < writeLen)
				writeLen = len;
			size_t compressedBytes = ZSTD_compressCCtx(_cctx, _compBuff.data(), _compBuff.size(), data, writeLen, _compressionLevel);
			if (ZSTD_isError(compressedBytes))
			{
				std::cerr << "error? " << ZSTD_getErrorName(compressedBytes) << std::endl;
				return false;
			}
			STREAM::write(_compBuff.data(), compressedBytes);

			data += writeLen;
			len -= writeLen;
		}
		return true;
	}

	void set_compression_level(int level)
	{
		if (level > 0)
			_compressionLevel = level;
	}

	template <typename INSTREAM>
	size_t compress(INSTREAM& raw, int compression_level=0)
	{
		if (!_cctx)
			return 0;
		set_compression_level(compression_level);

		std::array<char, CHUNK_SIZE> rawBuff;
		size_t totalBytesRead = 0;
		while (raw)
		{
			raw.read(rawBuff.data(), rawBuff.size());
			std::streamsize bytesRead = raw.gcount();
			if (bytesRead <= 0)
				break;
			totalBytesRead += bytesRead;

			if (!write(rawBuff.data(), bytesRead))
				break;
		}
		return totalBytesRead;
	}

	size_t pad(unsigned len)
	{
		if (len < 9)
			len = 9;

		// test what this actually does with ZSTD_isSkippableFrame and ZSTD_readSkippableFrame
		// then, replace it with ZSTD_writeSkippableFrame...
		std::string temp(len-8, '\0');
		size_t writ = ZSTD_writeSkippableFrame(_compBuff.data(), _compBuff.size(), temp.data(), temp.size(), 0);

		STREAM::write(_compBuff.data(), writ);
		return writ;
	}

	size_t write_header(const char* data, unsigned len)
	{
		std::string temp = "\x01";
		temp += std::string(data, len);
		size_t writ = ZSTD_writeSkippableFrame(_compBuff.data(), _compBuff.size(), temp.data(), temp.size(), 0);
		STREAM::write(_compBuff.data(), writ);
		return writ;
	}

	size_t size()
	{
		STREAM::seekg(0, std::ios::end);
		size_t len = STREAM::tellg();
		STREAM::seekg(0, std::ios::beg);
		return len;
	}

protected:
	static const size_t CHUNK_SIZE = 0x4000;
	int _compressionLevel = 16;
	ZSTD_CCtx* _cctx = ZSTD_createCCtx();
	std::vector<char> _compBuff = std::vector<char>(ZSTD_compressBound(CHUNK_SIZE));
};

}
