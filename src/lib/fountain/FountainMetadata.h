#pragma once

#include <array>

struct FountainMetadata
{
	static const unsigned md_size = 14;

	std::array<uint8_t,md_size> data = {0};

	FountainMetadata(uint32_t size, std::string name)
	{
		uint8_t* p = data.data();
		p[0] = (size >> 24) & 0xFF;
		p[1] = (size >> 16) & 0xFF;
		p[2] = (size >> 8) & 0xFF;
		p[3] = size & 0xFF;

		name.resize(md_size - 4);
		std::copy(name.begin(), name.end(), p+4);
	}

	FountainMetadata(const char* buff, unsigned len)
	{
		if (len > md_size)
			len = md_size;
		std::copy(buff, buff+len, data.data());
	}

	uint32_t file_size() const
	{
		uint32_t res = data[3];
		res |= ((uint32_t)data[2] << 8);
		res |= ((uint32_t)data[1] << 16);
		res |= ((uint32_t)data[0] << 24);
		return res;
	}

	std::string name() const
	{
		return std::string(data.data()+4, data.data()+md_size);
	}
};
