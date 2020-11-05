#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>

namespace rpi::__impl
{
    /*
        Template class responsible for queued callback 
        execution on a seperate thread.
    */
    template<typename _Fun>
    class dispatch_queue : private std::queue<_Fun>
    {
        std::future<void> dispatch_thread;  // Thread on which the functions are executed.
        std::mutex queue_access_mtx;        // Mutex for resource access control.

        // Method executes queued callback functions.
        void execute_tasks();

    public:

        // Constructor.
        dispatch_queue();
        // Destructor.
        ~dispatch_queue();
        // Push the function to the end of the queue.
        void push(const _Fun& fun);
    };

    template<typename _Fun>
    inline void dispatch_queue<_Fun>::execute_tasks()
    {
        std::unique_lock<std::mutex> lock{ queue_access_mtx };

        while (!std::queue<_Fun>::empty())
        {
            const _Fun& fun = (*this).front();
            (*this).pop();

            lock.unlock();    // Unlock the mutex while the callback is being executed.
            fun();            // Execute the callback function.
        }
    }

    template<typename _Fun>
    inline dispatch_queue<_Fun>::dispatch_queue() : dispatch_thread{}
    {
    }

    template<typename _Fun>
    inline dispatch_queue<_Fun>::~dispatch_queue()
    {
        {
            std::lock_guard<std::mutex> lock{ queue_access_mtx };

            while (!(*this).empty())
            {
                (*this).pop();
            }
        }

        if (dispatch_thread.valid())
        {
            dispatch_thread.wait();
        }
    }

    template<typename _Fun>
    inline void dispatch_queue<_Fun>::push(const _Fun& fun)
    {
        std::lock_guard<std::mutex> lock(queue_access_mtx);

        if ((*this).empty())
        {
            dispatch_thread = std::async(std::launch::async, [this]() { execute_tasks(); });
        }

        std::queue<_Fun>::push(fun);
    }
}