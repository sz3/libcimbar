/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#pragma once

#include <condition_variable>
#include <mutex>

namespace turbo {
class monitor
{
public:
	monitor()
		: _flag(false)
		, _cancelled(false)
	{}

	bool wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (_flag)
		{
			_flag = false;
			return !_cancelled;
		}
		_cv.wait(lock);
		return !_cancelled;
	}

	bool wait_for(unsigned ms)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (_flag)
		{
			_flag = false;
			return !_cancelled;
		}
		return _cv.wait_for(lock, std::chrono::milliseconds(ms)) != std::cv_status::timeout && !_cancelled;
	}

	void notify_one()
	{
		_cv.notify_one();
	}

	void notify_all()
	{
		_cv.notify_all();
	}

	void signal_all()
	{
		signal_all_internal(false);
	}

	void cancel()
	{
		signal_all_internal(true);
	}

protected:
	void signal_all_internal(bool cancelled)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		_flag = true;
		_cancelled = cancelled;
		notify_all();
	}

protected:
	bool _flag;
	bool _cancelled;
	std::condition_variable _cv;
	std::mutex _mutex;
};
} // namespace turbo
