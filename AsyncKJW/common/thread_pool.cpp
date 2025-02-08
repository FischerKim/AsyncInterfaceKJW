#pragma once
#include <pch.h>

namespace II
{
    thread_pool::thread_pool() : stop_flag(false)  // �⺻ ������. 4���� �����尡 �⺻��.
    {
        resize(4); 
    }

    thread_pool::thread_pool(int nThreads) : stop_flag(false) // ������. �μ��� ���� ��ŭ ������ ����.
    {
        resize(nThreads);  
    }

    thread_pool::~thread_pool()  // �ı���: ������ Stop �� Join ����.
    {
        stop(true); 
    }

    void thread_pool::resize(int nThreads) // ������ Ǯ�� �ִ� ������ ����
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

    void thread_pool::worker() // ������ ���� �Լ�
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

    void thread_pool::stop(bool waitForCompletion) // ������ Ǯ ����
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
