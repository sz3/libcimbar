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

	template <typename INSTREAM>
	size_t compress(INSTREAM& raw)
	{
		if (!_cctx)
			return 0;

		std::array<char, CHUNK_SIZE> rawBuff;
		size_t totalBytesRead = 0;
		while (raw)
		{
			raw.read(rawBuff.data(), rawBuff.size());
			std::streamsize bytesRead = raw.gcount();
			if (bytesRead <= 0)
				break;
			totalBytesRead += bytesRead;

			size_t compressedBytes = ZSTD_compressCCtx(_cctx, _compBuff.data(), _compBuff.size(), rawBuff.data(), (size_t)bytesRead, 6);
			if (ZSTD_isError(compressedBytes))
			{
				std::cout << "error? " << ZSTD_getErrorName(compressedBytes) << std::endl;
				break;
			}
			STREAM::write(_compBuff.data(), compressedBytes);
		}


		return totalBytesRead;
	}

protected:
	static const uint16_t CHUNK_SIZE = 0x4000;
	std::vector<char> _compBuff = std::vector<char>(ZSTD_compressBound(CHUNK_SIZE));
	ZSTD_CCtx* _cctx = ZSTD_createCCtx();
};

}
