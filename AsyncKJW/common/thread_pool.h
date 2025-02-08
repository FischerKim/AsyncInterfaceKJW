#pragma once

#ifndef MAX_QUEUE_SIZE
#define MAX_QUEUE_SIZE  100
#endif

namespace II
{
    class thread_pool {
    public:
        thread_pool();                                  // 기본 생성자
        thread_pool(int nThreads);                      // 생성자
        ~thread_pool();                                 // 파괴자

        void resize(int nThreads);                      // 쓰레드 풀에 있는 갯수를 조정

        template <typename F>
        void push(F&& f);                               // 수행할 task를 queue에다 쌓음.

        void stop(bool waitForCompletion = false);      // 쓰레드 풀 정지

    private:
        void worker();                                  // 쓰레드 수행 함수

        std::vector<std::thread> threads;               // 쓰레드 풀
        std::queue<std::function<void()>> task_queue;   // Task를 담는 Queue
        std::mutex queue_mutex;                         // queue 접근 용 Mutex
        std::condition_variable cv;                     // 특정 조건이 충족될 때까지 쓰레드를 차단하기 위해 동시 프로그래밍에서 사용되는 동기화 기본 요소
        std::atomic<bool> stop_flag;                    // 쓰레드 정지를 위한 Flag
    };

    template <typename F>
    void thread_pool::push(F&& f) // 수행할 task를 queue에다 쌓음.
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
