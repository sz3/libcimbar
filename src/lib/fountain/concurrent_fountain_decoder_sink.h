#pragma once

#include "fountain_decoder_sink.h"

#include "concurrentqueue/concurrentqueue.h"
#include <mutex>

class concurrent_fountain_decoder_sink
{
public:
	concurrent_fountain_decoder_sink(std::string data_dir, unsigned chunk_size)
		: _decoder(data_dir, chunk_size)
	{
	}

	bool good() const
	{
		return true;
	}

	unsigned chunk_size() const
	{
		return _decoder.chunk_size();
	}

	unsigned num_streams() const
	{
		return _decoder.num_streams();
	}

	unsigned num_done() const
	{
		return _decoder.num_done();
	}

	void process()
	{
		if (_mutex.try_lock())
		{
			std::string buff;
			while (_backlog.try_dequeue(buff))
				_decoder << buff;
			_mutex.unlock();
		}
	}

	bool write(const char* data, unsigned length)
	{
		std::string buffer(data, length);
		operator<<(buffer);
		return true;
	}

	concurrent_fountain_decoder_sink& operator<<(const std::string& buffer)
	{
		_backlog.enqueue(buffer);
		process();
		return *this;
	}

protected:
	std::mutex _mutex;
	fountain_decoder_sink _decoder;
	moodycamel::ConcurrentQueue< std::string > _backlog;
};
