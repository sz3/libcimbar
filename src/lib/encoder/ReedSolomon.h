/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

extern "C" {
    #include "libcorrect/include/correct.h"
}

// a wrapper for libcorrect's correct_reed_solomon_encode()

class ReedSolomon
{
public:
	struct BadChunk
	{
		unsigned size;

		explicit BadChunk(unsigned size)
		    : size(size)
		{}
	};

public:
	ReedSolomon(size_t parity_bytes)
	    : _parityBytes(parity_bytes)
	{
		_rs = correct_reed_solomon_create(correct_rs_primitive_polynomial_8_7_2_1_0, 1, 1, _parityBytes);
	}

	~ReedSolomon()
	{
		correct_reed_solomon_destroy(_rs);
	}

	unsigned parity() const
	{
		return _parityBytes;
	}

	ssize_t encode(const char* msg, unsigned msg_length, char* encoded)
	{
		return correct_reed_solomon_encode(_rs, reinterpret_cast<const uint8_t*>(msg), msg_length, reinterpret_cast<uint8_t*>(encoded));
	}

	ssize_t decode(const char* encoded, unsigned encoded_length, char* msg)
	{
		return correct_reed_solomon_decode(_rs, reinterpret_cast<const uint8_t*>(encoded), encoded_length, reinterpret_cast<uint8_t*>(msg));
	}

protected:
	correct_reed_solomon* _rs;
	unsigned _parityBytes;
};
