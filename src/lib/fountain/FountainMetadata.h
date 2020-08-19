/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

class FountainMetadata
{
public:
	static const unsigned md_size = 4;

public:
	FountainMetadata(uint64_t id)
	    : _data(id)
	{
	}

	FountainMetadata(const char* buff, unsigned len)
	{
		if (len > md_size)
			len = md_size;
		std::copy(buff, buff+len, data());
	}

	FountainMetadata(uint8_t encode_id, unsigned size)
	{
		uint8_t* d = data();
		d[0] = encode_id;
		d[1] = (size >> 16) & 0xFF;
		d[2] = (size >> 8) & 0xFF;
		d[3] = size & 0xFF;
	}

	unsigned id() const
	{
		return _data;
	}

	uint8_t encode_id() const
	{
		return data()[0];
	}

	unsigned file_size() const
	{
		const uint8_t* d = data();
		unsigned res = d[3];
		res |= ((uint32_t)d[2] << 8);
		res |= ((uint32_t)d[1] << 16);
		return res;
	}

protected:
	uint8_t* data()
	{
		return reinterpret_cast<uint8_t*>(&_data);
	}

	const uint8_t* data() const
	{
		return reinterpret_cast<const uint8_t*>(&_data);
	}

protected:
	uint64_t _data; // might invert this and only generate the uint64_t when we need it
};
