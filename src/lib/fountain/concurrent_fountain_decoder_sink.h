/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "fountain_decoder_sink.h"

#include "concurrentqueue/concurrentqueue.h"
#include <mutex>

template <typename OUTSTREAM>
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

	std::vector<std::string> get_done() const
	{
		std::lock_guard<std::mutex> lock(_readMutex);
		return _done;
	}

	std::vector<double> get_progress() const
	{
		std::lock_guard<std::mutex> lock(_readMutex);
		return _progress;
	}

	void update_status()
	{
		// we call this under the writeMutex+readMutex. The `const`s are only under readMutex.
		// just thought you ought to know
		std::lock_guard<std::mutex> lock(_readMutex);
		_done = _decoder.get_done();
		_progress = _decoder.get_progress();
	}

	void process()
	{
		if (_writeMutex.try_lock())
		{
			// maybe a 2nd mutex for retrieving _decoder.has_file()??
			std::string buff;
			while (_backlog.try_dequeue(buff))
				_decoder << buff;

			update_status();
			_writeMutex.unlock();
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
	std::mutex _writeMutex;
	mutable std::mutex _readMutex;
	fountain_decoder_sink<OUTSTREAM> _decoder;
	moodycamel::ConcurrentQueue< std::string > _backlog;

	std::vector<std::string> _done;
	std::vector<double> _progress;
};
