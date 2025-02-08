#pragma once
#include <pch.h>

namespace II
{
    thread_pool::thread_pool() : stop_flag(false)  // 기본 생성자. 4개의 스레드가 기본값.
    {
        resize(4); 
    }

    thread_pool::thread_pool(int nThreads) : stop_flag(false) // 생성자. 인수의 갯수 만큼 스레드 생성.
    {
        resize(nThreads);  
    }

    thread_pool::~thread_pool()  // 파괴자: 쓰레드 Stop 및 Join 수행.
    {
        stop(true); 
    }

    void thread_pool::resize(int nThreads) // 쓰레드 풀에 있는 갯수를 조정
    { 
        stop_flag = false;
        for (auto& t : threads) 
        {
            if (t.joinable()) 
            {
                t.join(); 
            }
        }
        threads.clear();

        for (int i = 0; i < nThreads; ++i) 
        {
            threads.push_back(std::thread(&thread_pool::worker, this));
        }
    }

    void thread_pool::worker() // 쓰레드 수행 함수
    {
        while (!stop_flag) 
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this] { return !task_queue.empty() || stop_flag; });
                if (stop_flag && task_queue.empty()) 
                {
                    return;
                }

                task = std::move(task_queue.front());
                task_queue.pop();
            }

            task(); 
        }
    }

    void thread_pool::stop(bool waitForCompletion) // 쓰레드 풀 정지
    {
        stop_flag = true;
        cv.notify_all();

        if (waitForCompletion) 
        {
            for (auto& t : threads) 
            {
                if (t.joinable()) 
                {
                    t.join();
                }
            }
        }
    }
}
