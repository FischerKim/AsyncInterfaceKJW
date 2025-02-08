#pragma once

#ifndef MAX_QUEUE_SIZE
#define MAX_QUEUE_SIZE  100
#endif

namespace II
{
    class thread_pool {
    public:
        thread_pool();                                  // �⺻ ������
        thread_pool(int nThreads);                      // ������
        ~thread_pool();                                 // �ı���

        void resize(int nThreads);                      // ������ Ǯ�� �ִ� ������ ����

        template <typename F>
        void push(F&& f);                               // ������ task�� queue���� ����.

        void stop(bool waitForCompletion = false);      // ������ Ǯ ����

    private:
        void worker();                                  // ������ ���� �Լ�

        std::vector<std::thread> threads;               // ������ Ǯ
        std::queue<std::function<void()>> task_queue;   // Task�� ��� Queue
        std::mutex queue_mutex;                         // queue ���� �� Mutex
        std::condition_variable cv;                     // Ư�� ������ ������ ������ �����带 �����ϱ� ���� ���� ���α׷��ֿ��� ���Ǵ� ����ȭ �⺻ ���
        std::atomic<bool> stop_flag;                    // ������ ������ ���� Flag
    };

    template <typename F>
    void thread_pool::push(F&& f) // ������ task�� queue���� ����.
    {
        if (task_queue.size() < MAX_QUEUE_SIZE) 
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                task_queue.push(std::forward<F>(f));
            }
            cv.notify_one();
        }
        else 
        {
            std::cerr << "thread_pool:: Queue is full, dropping task\n";
        }
    }
}
