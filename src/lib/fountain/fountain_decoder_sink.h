#pragma once

#include "fountain_decoder_stream.h"
#include "FountainMetadata.h"
#include "serialize/format.h"
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>

// what we're trying to do here is have an object that can accept a complete (~8400 byte) buffer,
// pull out the header, and give the appropriate decoder its bytes

class fountain_decoder_sink
{
public:
	fountain_decoder_sink(std::string data_dir, unsigned buffer_size)
	    : _dataDir(data_dir)
	    , _bufferSize(buffer_size)
	{
	}

	unsigned chunk_size() const
	{
		return _bufferSize;
	}

	unsigned md_size() const
	{
		return FountainMetadata::md_size;
	}

	bool store(const std::string& name, const std::vector<uint8_t>& data)
	{
		std::string file_path = fmt::format("{}/{}", _dataDir, name);
		std::ofstream f(file_path);
		f.write((char*)data.data(), data.size());
		return true;
	}

	void mark_done(const std::string& name, unsigned size)
	{
		_done.insert({name, size});
		auto it = _streams.find(name);
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

	bool is_done(const std::string& name, unsigned size) const
	{
		return _done.find({name, size}) != _done.end();
	}

	bool decode_frame(const char* data, unsigned size)
	{
		if (size < md_size())
			return false;

		FountainMetadata md(data, size);
		if (!md.file_size())
			return false;

		data += md_size();
		size -= md_size();

		// check if already done
		if (is_done(md.name(), md.file_size()))
			return false;

		// find or create
		auto p = _streams.emplace(std::piecewise_construct, std::make_tuple(md.name()), std::make_tuple(md.file_size(), _bufferSize));
		fountain_decoder_stream& s = p.first->second;
		if (s.data_size() != md.file_size())
			return false;

		auto finished = s.write(data, size);
		if (!finished)
			return false;

		if (store(md.name(), *finished))
			mark_done(md.name(), md.file_size());
		return true;
	}

	fountain_decoder_sink& operator<<(const std::string& buffer)
	{
		decode_frame(buffer.data(), buffer.size());
		return *this;
	}

protected:
	std::string _dataDir;
	unsigned _bufferSize;
	std::map<std::string, fountain_decoder_stream> _streams;
	std::set<std::pair<std::string, unsigned>> _done;
};
