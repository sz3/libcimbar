/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "fountain_decoder_stream.h"
#include "FountainMetadata.h"
#include "serialize/format.h"

#include <cstdio>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename OUTSTREAM>
class fountain_decoder_sink
{
public:
	fountain_decoder_sink(std::string data_dir, unsigned chunk_size, bool log_writes=false)
		: _dataDir(data_dir)
		, _chunkSize(chunk_size)
		, _logWrites(log_writes)
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
		std::string file_path = fmt::format("{}/{}", _dataDir, get_filename(md));
		OUTSTREAM f(file_path);
		f.write((char*)data.data(), data.size());
		if (_logWrites)
			printf("%s\n", file_path.c_str());
		return true;
	}

	void mark_done(const FountainMetadata& md)
	{
		_done.insert(md.id());
		auto it = _streams.find(stream_slot(md));
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

	std::vector<std::string> get_done() const
	{
		std::vector<std::string> done;
		for (uint32_t id : _done)
			done.push_back( get_filename(FountainMetadata(id)) );
		return done;
	}

	std::vector<double> get_progress() const
	{
		std::vector<double> progress;
		for (auto&& [slot, s] : _streams)
		{
			unsigned br = s.blocks_required();
			if (br)
				progress.push_back( s.progress() * 1.0 / s.blocks_required() );
		}
		return progress;
	}

	bool is_done(uint32_t id) const
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
			mark_done(md);
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
	// streams is limited to at most 8 decoders at a time. Currently, we just use the lower bits of the encode_id.
	uint8_t stream_slot(const FountainMetadata& md) const
	{
		return md.encode_id() & 0x7;
	}

	std::string get_filename(const FountainMetadata& md) const
	{
		return fmt::format("{}.{}", md.encode_id(), md.file_size());
	}

protected:
	std::string _dataDir;
	unsigned _chunkSize;

	// maybe instead of unordered_map+set, something where we can "age out" old streams?
	// e.g. most recent 16/8, or something?
	// question is what happens to _done/_streams when we wrap for continuous data streaming...
	std::unordered_map<uint8_t, fountain_decoder_stream> _streams;
	// track the uint32_t combo of (encode_id,size) to avoid redundant work
	std::set<uint32_t> _done;
	bool _logWrites;
};
