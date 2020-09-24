/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "fountain_decoder_stream.h"
#include "FountainMetadata.h"
#include "serialize/format.h"

#include <set>
#include <string>
#include <unordered_map>
#include <utility>

// what we're trying to do here is have an object that can accept a complete (~8400 byte) buffer,
// pull out the header, and give the appropriate decoder its bytes

template <typename OUTSTREAM>
class fountain_decoder_sink
{
public:
	fountain_decoder_sink(std::string data_dir, unsigned chunk_size)
	    : _dataDir(data_dir)
	    , _chunkSize(chunk_size)
	{
	}

	bool good() const
	{
		return true;
	}

	unsigned chunk_size() const
	{
		return _chunkSize;
	}

	bool store(const FountainMetadata& md, const std::vector<uint8_t>& data)
	{
		std::string file_path = fmt::format("{}/{}.{}", _dataDir, md.encode_id(), md.file_size());
		OUTSTREAM f(file_path);
		f.write((char*)data.data(), data.size());
		return true;
	}

	void mark_done(uint64_t id)
	{
		_done.insert(id);
		auto it = _streams.find(id);
		if (it != _streams.end())
			_streams.erase(it);
	}

	unsigned num_streams() const
	{
		return _streams.size();
	}

	unsigned num_done() const
	{
		return _done.size();
	}

	bool is_done(uint64_t id) const
	{
		return _done.find(id) != _done.end();
	}

	bool decode_frame(const char* data, unsigned size)
	{
		if (size < FountainMetadata::md_size)
			return false;

		FountainMetadata md(data, size);
		if (!md.file_size())
			return false;

		// check if already done
		if (is_done(md.id()))
			return false;

		// find or create
		auto p = _streams.try_emplace(stream_slot(md), md.file_size(), _chunkSize);
		fountain_decoder_stream& s = p.first->second;
		if (s.data_size() != md.file_size())
			return false;

		auto finished = s.write(data, size);
		if (!finished)
			return false;

		if (store(md, *finished))
			mark_done(md.id());
		return true;
	}

	bool write(const char* data, unsigned length)
	{
		return decode_frame(data, length);
	}

	fountain_decoder_sink& operator<<(const std::string& buffer)
	{
		decode_frame(buffer.data(), buffer.size());
		return *this;
	}

protected:
	// streams is limited to at most 8 decoders at a time. Current, we just use the lower bits of the encode_id.
	uint8_t stream_slot(const FountainMetadata& md) const
	{
		return md.encode_id() & 0x7;
	}

protected:
	std::string _dataDir;
	unsigned _chunkSize;

	std::unordered_map<uint8_t, fountain_decoder_stream> _streams;
	// track the uint64_t combo of (encode_id,size) to avoid redundant work
	std::set<uint64_t> _done;
};
