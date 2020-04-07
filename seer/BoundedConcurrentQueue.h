#pragma once

#include <blockingconcurrentqueue.h>
#include <thread>

namespace seer {

template <typename T>
class BoundedConcurrentQueue {
    moodycamel::BlockingConcurrentQueue<T> _queue;
    size_t _size;

public:
    BoundedConcurrentQueue(size_t size = 1) : _size(size) {}

    void enqueue(T item) {
        while (_queue.size_approx() > _size) {
            std::this_thread::yield();
        }
        _queue.enqueue(std::forward<T>(item));
    }

    void dequeue(T& item) {
        _queue.wait_dequeue(item);
    }
};

} // namespace seer
