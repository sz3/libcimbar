#pragma once

extern "C" {
	#include "libcorrect/include/correct.h"
}

// a wrapper for libcorrect's correct_reed_solomon_encode()

class ReedSolomon
{
public:
	ReedSolomon(size_t parity_bytes)
		: _parityBytes(parity_bytes)
	{
		_rs = correct_reed_solomon_create(correct_rs_primitive_polynomial_ccsds, 1, 1, _parityBytes);
	}

	~ReedSolomon()
	{
		correct_reed_solomon_destroy(_rs);
	}

	size_t parity() const
	{
		return _parityBytes;
	}

	ssize_t encode(const uint8_t* msg, size_t msg_length, uint8_t* encoded)
	{
		return correct_reed_solomon_encode(_rs, msg, msg_length, encoded);
	}

protected:
	correct_reed_solomon* _rs;
	size_t _parityBytes;
};
