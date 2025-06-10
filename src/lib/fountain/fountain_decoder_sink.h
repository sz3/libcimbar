/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "fountain_decoder_stream.h"
#include "FountainMetadata.h"
#include "serialize/format.h"

#include <cstdio>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename OUTSTREAM>
std::function<void(const std::string&, const std::vector<uint8_t>&)> write_on_store(std::string data_dir, bool log_writes=false)
{
	return [data_dir, log_writes](const std::string& filename, const std::vector<uint8_t>& data)
	{
		std::string file_path = fmt::format("{}/{}", data_dir, filename);
		OUTSTREAM f(file_path, std::ios::binary);
		f.write((char*)data.data(), data.size());
		if (log_writes)
			printf("%s\n", file_path.c_str());
	};
}

class fountain_decoder_sink
{
public:
	fountain_decoder_sink(unsigned chunk_size, const std::function<void(const std::string&, const std::vector<uint8_t>&)>& on_store=nullptr)
		: _chunkSize(chunk_size)
		, _onStore(on_store)
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

	std::string get_filename(const FountainMetadata& md) const
	{
		return fmt::format("{}.{}", md.encode_id(), md.file_size());
	}

	bool store(const FountainMetadata& md, fountain_decoder_stream& s)
	{
		if (_onStore)
		{
			auto res = s.recover();
			if (!res)
				return false;
			_onStore(get_filename(md), *res);
			mark_done(md);
		}
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

	int64_t decode_frame(const char* data, unsigned size)
	{
		if (size < FountainMetadata::md_size)
			return -10;

		FountainMetadata md(data, size);
		if (!md.file_size())
		{
			/*std::cout << fmt::format("decode frame {} ... {},{},{},{}",
									 md.file_size(), (unsigned)data[0], (unsigned)data[1], (unsigned)data[2], (unsigned)data[3]) << std::endl;*/
			return -11;
		}

		// check if already done
		if (is_done(md.id()))
			return -1;

		// find or create
		auto p = _streams.try_emplace(stream_slot(md), md.file_size(), _chunkSize);
		fountain_decoder_stream& s = p.first->second;
		if (s.data_size() != md.file_size())
			return -12;

		bool finished = s.write(data, size);
		if (!finished)
			return 0;

		// when you provide a write callback,
		// store() will call mark_done() afterwards
		// -- and the assembled file will we dropped from RAM.
		// but if no callback is provided, you can do something else.
		store(md, s);
		return md.id();
	}

	bool write(const char* data, unsigned length)
	{
		return decode_frame(data, length) > 0;
	}

	fountain_decoder_sink& operator<<(const std::string& buffer)
	{
		write(buffer.data(), buffer.size());
		return *this;
	}

	bool recover(uint32_t id, unsigned char* data, unsigned size)
	{
		// iff you don't provide a write callback
		// this finalizes the write.
		// after the data is copied to `data`,
		// the stream will be dropped from RAM (`mark_done()`)
		FountainMetadata md(id);
		auto p = _streams.find(stream_slot(md));
		if (p == _streams.end())
			return false;

		fountain_decoder_stream& s = p->second;
		bool res = s.recover(data, size);
		mark_done(md);
		return res;
	}

protected:
	// streams is limited to at most 8 decoders at a time. Currently, we just use the lower bits of the encode_id.
	uint8_t stream_slot(const FountainMetadata& md) const
	{
		return md.encode_id() & 0x7;
	}

protected:
	unsigned _chunkSize;
	std::function<void(const std::string&, const std::vector<uint8_t>&)> _onStore;

	// maybe instead of unordered_map+set, something where we can "age out" old streams?
	// e.g. most recent 16/8, or something?
	// question is what happens to _done/_streams when we wrap for continuous data streaming...
	std::unordered_map<uint8_t, fountain_decoder_stream> _streams;
	// track the uint32_t combo of (encode_id,size) to avoid redundant work
	std::set<uint32_t> _done;
	bool _logWrites;
};
