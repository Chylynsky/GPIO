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
	/*
		Template class responsible for queued callback 
		execution on a seperate thread.
	*/
	template<typename _Fun>
	class __dispatch_queue
	{
		std::queue<_Fun> function_queue;			// Queue with functions to be executed.
		std::future<void> dispatch_thread;				// Thread on which the functions are executed.
		std::mutex queue_access_mtx;				// Mutex for resource access control.
		std::condition_variable queue_empty_cond;	// Puts the thread to sleep when no functions are in the queue.
		std::atomic<bool> dispatch_thread_exit;		// Thread loop control.

		// Method executes queued callback functions.
		void execute_tasks();

	public:

		// Constructor.
		__dispatch_queue();
		// Destructor.
		~__dispatch_queue();
		// Push the function to the end of the queue.
		void push(const _Fun& fun);
	};

	template<typename _Fun>
	void __dispatch_queue<_Fun>::execute_tasks()
	{
		while (!dispatch_thread_exit)
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);

			// Execute all functions
			while (!function_queue.empty())
			{
				_Fun fun = function_queue.front();
				
				lock.unlock();	// Unlock the mutex while the callback is being executed.
				fun();			// Execute the callback function.
				lock.lock();	// Lock again.

				function_queue.pop();
			}

			// Wait until queue is not empty or destructor is called.
			queue_empty_cond.wait(lock, [this]() { return (!function_queue.empty() || dispatch_thread_exit); });
		}
	}

	template<typename _Fun>
	__dispatch_queue<_Fun>::__dispatch_queue() : dispatch_thread_exit{ false }
	{
		dispatch_thread = std::async(std::launch::async, [this]() { execute_tasks(); });
	}

	template<typename _Fun>
	__dispatch_queue<_Fun>::~__dispatch_queue()
	{
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);
			dispatch_thread_exit = true;
		} // Release the mutex and notify wating thread.
		queue_empty_cond.notify_one();

		// Wait for the thread to exit.
		dispatch_thread.get();
	}

	template<typename _Fun>
	void __dispatch_queue<_Fun>::push(const _Fun& fun)
	{
		{
			std::unique_lock<std::mutex> lock(queue_access_mtx);
			function_queue.push(fun);
		} // Release the mutex and notify wating thread.
		queue_empty_cond.notify_one();
	}
}