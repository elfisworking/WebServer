//
// Created by yumin_zhang on 2022/4/11.
//

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>
#include <assert.h>

template<class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxCapacity = 1000);
    ~BlockQueue();
    void clear();
    bool empty();
    bool full();
    void close();
    size_t size();
    size_t capacity();
    T front();
    T back();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);
    void flush();
private:
    std::deque<T> deq;
    size_t cap; // capacity
    std::mutex mtx;
    bool isClose;
    std::condition_variable condConsumer;
    std::condition_variable condProducer;
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxCapacity) : cap(maxCapacity){
    assert(maxCapacity > 0);
    isClose = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue<T>() {
    // why ?
    close();
}

template<typename T>
void BlockQueue<T>::close() {
    {
        std::lock_guard<std::mutex> locker(mtx);
        isClose = true;
        deq.clear();
    }
    // notify consumer and producer that deq has closed
    condConsumer.notify_all();
    condProducer.notify_all();
}

template<typename T>
void BlockQueue<T>::flush() {
    condConsumer.notify_one();
}

template<typename T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx);
    deq.clear();
}

template<typename T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> locker(mtx);
    T item = deq.front();
    return item;
}

template<typename T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.back();
}

template<typename T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.size();
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx);
    return cap;
}

template<typename T>
void BlockQueue<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.size() >= cap){
        condProducer.wait(locker);
    }
    deq.push_back(item);
    condConsumer.notify_one();
}

template<typename T>
void BlockQueue<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.size() >= cap) {
        condProducer.wait(locker);
    }
    deq.push_front(item);
    condConsumer.notify_one();
}

template<typename T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.empty();
}

template<typename T>
bool BlockQueue<T>::full() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.size() >= cap;
}

template<typename  T>
bool BlockQueue<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx);
     while(deq.empty()) {
         condConsumer.wait(locker);
         if(isClose) {
             return false;
         }
     }
     item = deq.front();
     deq.pop_front();
     return true;
}

template<typename T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.empty()) {
        if(condConsumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            // timeout
            return false;
        }
        if(isClose) {
            return false;
        }
    }
    item = deq.front();
    deq.pop_front();
    return true;
}

#endif
