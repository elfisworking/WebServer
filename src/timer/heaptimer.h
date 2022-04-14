//
// Created by yumin_zhang on 2022/4/12.
//

#ifndef HEAPTIMER_H
#define HEAPTIMER_H
#include <time.h>
#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <assert.h>
#include <queue>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock CLOCK;
typedef std::chrono::milliseconds MS;
typedef CLOCK::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallback  callback;
    bool operator<(const TimerNode& t){
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();
    void adjust(int id, int timeout);
    void add(int id, int timeout, const TimeoutCallback & cb);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int getNextTick();
private:
    void del(size_t i);
    // what't meaning of this function
    void siftup(size_t i);
    bool siftdown(size_t index, size_t n);
    void swapNode(size_t i, size_t j);
    std::vector<TimerNode> heap;
    std::unordered_map<int, size_t> ref;
};


#endif //HEAPTIMER_H
