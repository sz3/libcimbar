#pragma once

#include <vector>

template <typename STREAM>
class aligned_stream
{
public:
	aligned_stream(STREAM& stream, unsigned align_increment, unsigned align_offset)
		: _stream(stream)
		, _buffer(align_increment, 0)
		, _offset(0)
		, _alignOffset(align_offset)
		, _alignIncrement(align_increment)
		, _badChunk(false)
	{
	}

	long tellp() const
	{
		return _stream.tellp();
	}

	// this is primarily a pass-through
	// for first align_offset bytes, pass through
	// if we have a complete increment, pass through
	// otherwise, buffer
	// if we get a bad chunk, discard buffer, and mark buffer as dirty?
	aligned_stream& write(const char* data, unsigned length)
	{
		// also have the header to contend with
		while (length > 0)
		{
			// header -- first _alignOffset bits are unconditionally written to stream
			if (_offset < _alignOffset)
			{
				unsigned writeLen = std::min(_alignOffset - _offset, length);
				_stream.write(data, writeLen);
				length -= writeLen;
				data += writeLen;
				_alignOffset -= writeLen;
				continue;
			}

			// else, body of message

			// if we have a full write
			unsigned work = length + _offset;
			if (work >= _alignIncrement)
			{
				unsigned writeLen = _alignIncrement - _offset;
				if (_badChunk)
				{
					_badChunk = false;
					_offset = 0;
				}
				else
				{
					flush();
					_stream.write(data, writeLen);
				}

				length -= writeLen;
				data += writeLen;
				continue;
			}

			// if we need to store it for later
			unsigned writeLen = length;
			std::copy(data, data+writeLen, _buffer.data()+_offset);
			_offset += writeLen;
			length = 0;
		}
		return *this;
	}

	void mark_bad_chunk(unsigned len)
	{
		_badChunk = true;
		_offset += len;
		_offset = _offset % _alignIncrement;
	}

	void flush()
	{
		if (_offset > 0)
			_stream.write(_buffer.data(), _offset);
		_offset = 0;
	}


protected:
	STREAM& _stream;
	std::vector<char> _buffer;
	unsigned _offset;
	unsigned _alignOffset;
	unsigned _alignIncrement;
	bool _badChunk;
};

