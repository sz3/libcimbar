#pragma once

#include "zstd/zstd.h"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>

//template <typename STREAM>
class zstd_compressor : public std::stringstream
{
public:
	template <typename INSTREAM>
	size_t compress(INSTREAM& raw)
	{
		// ????
		std::array<char, 0x4000> rawBuff;
		std::vector<char> compBuff(ZSTD_compressBound(0x4000));

		ZSTD_CCtx* ctx = ZSTD_createCCtx();
		if (!ctx)
			return 0;

		size_t totalBytesRead = 0;
		while (raw)
		{
			raw.read(rawBuff.data(), rawBuff.size());
			std::streamsize bytesRead = raw.gcount();
			if (bytesRead <= 0)
				break;

			totalBytesRead += bytesRead;
			size_t compressedBytes = ZSTD_compressCCtx(ctx, compBuff.data(), compBuff.size(), rawBuff.data(), (size_t)bytesRead, 6);
			if (ZSTD_isError(compressedBytes))
			{
				std::cout << "error? " << ZSTD_getErrorName(compressedBytes) << std::endl;
				break;
			}
			this->write(compBuff.data(), compressedBytes);
		}

		ZSTD_freeCCtx(ctx);
		return totalBytesRead;
	}
};
