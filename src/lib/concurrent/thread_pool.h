/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include "monitor.h"
#include "concurrentqueue/concurrentqueue.h"
#include <atomic>
#include <functional>
#include <list>
#include <thread>

namespace turbo {
class thread_pool
{
public:
	thread_pool(unsigned numThreads = std::thread::hardware_concurrency());
	thread_pool(unsigned numThreads, unsigned producerLimit); // tries to limit size of queue
	~thread_pool();

	bool start();
	void stop();
	void pause();
	void resume();

	void execute(std::function<void()> fun);
	bool try_execute(std::function<void()> fun);
	size_t queued() const;

protected:
	void run();

protected:
	std::atomic<int> _running = 0;
	std::atomic<bool> _pause = false;
	unsigned _numThreads;
	std::list<std::thread> _threads;

	turbo::monitor _notifyRunning;
	turbo::monitor _notifyWork;
	moodycamel::ConcurrentQueue< std::function<void()> > _queue;
};

inline thread_pool::thread_pool(unsigned numThreads)
    : _numThreads(numThreads)
    , _queue()
{
}
inline thread_pool::thread_pool(unsigned numThreads, unsigned producerLimit)
    : _numThreads(numThreads)
    , _queue(1, 0, producerLimit)
{
}

inline thread_pool::~thread_pool()
{
	stop();
}

inline bool thread_pool::start()
{
	if (_running > 0)
		return true;

	for (unsigned i = 0; i < _numThreads; ++i)
		_threads.push_back( std::thread(std::bind(&thread_pool::run, this)) );
	_notifyRunning.wait_for(10000);
	return _running == _numThreads;
}

inline void thread_pool::stop()
{
	_running = -1 - _numThreads;
	_notifyWork.signal_all();
	for (std::list<std::thread>::iterator it = _threads.begin(); it != _threads.end(); ++it)
	{
		if (it->joinable())
			it->join();
	}
	_threads.clear();
}

inline void thread_pool::pause()
{
	_pause = true;
}

inline void thread_pool::resume()
{
	_pause = false;
	_notifyWork.notify_all();
}

inline void thread_pool::execute(std::function<void()> fun)
{
	_queue.enqueue(fun);
	_notifyWork.notify_one();
}

inline bool thread_pool::try_execute(std::function<void()> fun)
{
	bool success = _queue.try_enqueue(fun);
	if (success)
		_notifyWork.notify_one();
	return success;
}

inline size_t thread_pool::queued() const
{
	return _queue.size_approx();
}

inline void thread_pool::run()
{
	if (++_running == _numThreads)
		_notifyRunning.signal_all();
	while (_running > 0 and !_pause)
	{
		_notifyWork.wait();
		if (_running <= 0)
			break;

		std::function<void()> fun;
		while (!_pause and _queue.try_dequeue(fun))
		{
			if (_running <= 0)
				break;
			fun();
		}
	}
}

} // namespace turbo
