//
// Created by yumin_zhang on 2022/4/12.
//

#include "heaptimer.h"
HeapTimer::HeapTimer() {
    heap.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}

void HeapTimer::clear() {
    heap.clear();
    ref.clear();
}

void HeapTimer::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < heap.size());
    assert(j >= 0 && j < heap.size());
    std::swap(heap[i], heap[j]);
    ref[heap[i].id] = i;
    ref[heap[j].id] = j;
}
// heap is [0, n)
bool HeapTimer::siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < heap.size());
    assert(n >=0 && n <= heap.size());
    size_t i = index;
    size_t j = 2 * index + 1;
    while(j < n) {
        if(j + 1 < n && heap[j + 1] < heap[j]) j++;
        if(heap[i] < heap[j]) break;
        swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::siftup(size_t i) {
    assert(i >= 0 && i < heap.size());
    size_t j = (i - 1) / 2;// parent node
    while(j >= 0) {
        if(heap[j] < heap[i]) break;
        swapNode(i, j);
        i = j;
        j = (j - 1) / 2;
    }
}

void HeapTimer::add(int id, int timeout, const TimeoutCallback &cb) {
    assert(id > 0);
    assert(timeout > 0);
    size_t index;
    if(ref.count(id)) {
        index = heap.size();
        ref[id] = index;
        heap.push_back({id, CLOCK::now() + MS(timeout), cb});
        siftup(index);
        // adjust
    } else {
        // add
        index = ref[id];
        heap[index].expires = CLOCK::now() + MS(timeout);// update expire time
        heap[index].callback = cb;
        // first siftdown, if siftdown function return false, do siftup
        if(!siftdown(index, heap.size())) {
            siftup(index);
        }
    }
}

void HeapTimer::del(size_t i) {
    assert(!heap.empty() && i >= 0 && i < heap.size());
    size_t swapNodeIndex = heap.size() - 1;
    if(i < swapNodeIndex) {
        swapNode(i, swapNodeIndex);
        if(!siftdown(i, swapNodeIndex)) {
            siftup(i);
        }
    }
    ref.erase(heap.back().id);
    heap.pop_back();
}

void HeapTimer::adjust(int id, int timeout) {
    assert(!heap.empty() && ref.count(id) > 0);
    heap[ref[id]].expires = CLOCK::now() + MS(timeout);
    siftdown(ref[id], heap.size());
}

void HeapTimer::doWork(int id) {
    assert(!heap.empty() && ref.count(id) > 0);
    auto timeNode = heap[ref[id]];
    timeNode.callback();
    del(ref[id]);

}

void HeapTimer::pop() {
    assert(!heap.empty());
    del(0);
}

void HeapTimer::tick() {
    // clear timeout node
    if(heap.empty()) {
        return;
    }
    while(!heap.empty()) {
        auto node = heap.front();
        if(std::chrono::duration_cast<MS>(node.expires - CLOCK::now()).count() > 0) {
            break;
        }
        node.callback();
        pop();
    }
}

int HeapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!heap.empty()) {
        res = std::chrono::duration_cast<MS>(heap.front().expires - CLOCK::now()).count();
        if(res < 0) res = 0;
    }
    return res;
}