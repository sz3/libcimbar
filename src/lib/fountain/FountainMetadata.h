/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <array>

class FountainMetadata
{
protected:
	static void update_block_id_internal(uint16_t block_id, uint8_t* arr)
	{
		arr[0] = (block_id >> 8) & 0xFF;
		arr[1] = block_id & 0xFF;
	}

public:
	static const unsigned md_size = 6;

	static void to_uint8_arr(uint8_t encode_id, unsigned size, uint16_t block_id, uint8_t* arr)
	{
		arr[0] = (encode_id & 0x7F) | ((size >> 17) & 0x80);
		arr[1] = (size >> 16) & 0xFF;
		arr[2] = (size >> 8) & 0xFF;
		arr[3] = size & 0xFF;
		update_block_id_internal(block_id, arr+4);
	}

public:
	FountainMetadata(uint32_t id)
	{
		uint8_t* d = _data.data();
		std::copy(reinterpret_cast<uint8_t*>(&id), reinterpret_cast<uint8_t*>(&id)+4, d);
		d[4] = d[5] = 0;
	}

	explicit FountainMetadata(const char* buff, unsigned len)
	{
		if (len > md_size)
			len = md_size;
		std::copy(buff, buff+len, _data.data());
	}

	FountainMetadata(uint8_t encode_id, unsigned size, uint16_t block_id)
	{
		uint8_t* d = _data.data();
		to_uint8_arr(encode_id, size, block_id, d);
	}

	uint32_t id() const
	{
		uint32_t shortid = 0;
		std::copy(data(), data()+4, reinterpret_cast<uint8_t*>(&shortid));
		return shortid;
	}

	uint8_t encode_id() const
	{
		return data()[0] & 0x7F;
	}

	uint16_t block_id() const
	{
		const uint8_t* d = data()+4;
		uint16_t res = d[1];
		res |= ((uint16_t)d[0] << 8);
		return res;
	}

	void increment_block_id()
	{
		update_block_id_internal(block_id()+1, _data.data()+4);
	}

	unsigned file_size() const
	{
		const uint8_t* d = data();
		unsigned res = d[3];
		res |= ((uint32_t)d[2] << 8);
		res |= ((uint32_t)d[1] << 16);
		res |= ((uint32_t)d[0] & 0x80) << 17;
		return res;
	}

	const uint8_t* data() const
	{
		return const_cast<const uint8_t*>(_data.data());
	}

protected:
	std::array<uint8_t, 6> _data;
};
