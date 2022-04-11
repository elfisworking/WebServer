//
// Created by yumin_zhang on 2022/4/9.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <assert.h>
class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 8);
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    ~ThreadPool();

    template<class T>
    void addTask(T&& task) {
        {
           std::lock_guard<std::mutex> locker(pool->mtx);
           pool->tasks.emplace(std::forward<T>(task));
        }
        pool->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool;

};


#endif //MYWEBSERVER_THREADPOOL_H
