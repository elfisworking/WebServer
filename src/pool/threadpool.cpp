//
// Created by yumin_zhang on 2022/4/9.
//

#include "threadpool.h"
ThreadPool::ThreadPool(size_t threadCount): pool(std::make_shared<Pool>()) {
    assert(threadCount > 0);
    for(size_t i = 0; i < threadCount; i++) {
        std::thread([p = pool] {
            std::unique_lock<std::mutex> locker(p->mtx);
            while(true) {
                if(!p->tasks.empty()) {
                    auto task = std::move(p->tasks.front());
                    p->tasks.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                }
                // 
                else if(p->isClosed) break;
                // 条件变量释放 当前任务队列中没有任务时
                else p->cond.wait(locker);
            }
        }).detach();
    }
}

ThreadPool::~ThreadPool() {
    if(static_cast<bool>(pool)) {
        {
            std::lock_guard<std::mutex> locker(pool->mtx);
            pool->isClosed = true;
        }
        pool->cond.notify_all();
    }
}
