#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>

namespace rpi
{
	template<typename _Fun>
	class dispatch_queue
	{
		std::queue<_Fun> function_queue;
		std::thread dispatch_thread;
		std::mutex queue_access_mtx;
		std::condition_variable queue_empty_cond;
		bool dispatch_thread_exit;

		void run();

	public:

		dispatch_queue();
		~dispatch_queue();

		void push(const _Fun& fun);
	};

	template<typename _Fun>
	void dispatch_queue<_Fun>::run()
	{
		while (!dispatch_thread_exit)
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);

			while (!function_queue.empty())
			{
				_Fun fun = function_queue.front();
				fun();
				function_queue.pop();
			}

			queue_empty_cond.wait(lock, [this] { return (!function_queue.empty() || dispatch_thread_exit); });
		}
	}

	template<typename _Fun>
	dispatch_queue<_Fun>::dispatch_queue() : dispatch_thread_exit{ false }
	{
		dispatch_thread = std::thread(std::bind(&dispatch_queue::run, this));
	}

	template<typename _Fun>
	dispatch_queue<_Fun>::~dispatch_queue()
	{
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);
			dispatch_thread_exit = true;
		}
		queue_empty_cond.notify_one();

		if (dispatch_thread.joinable())
		{
			dispatch_thread.join();
		}
	}

	template<typename _Fun>
	void dispatch_queue<_Fun>::push(const _Fun& fun)
	{
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);
			function_queue.push(fun);
		}
		queue_empty_cond.notify_one();
	}
}