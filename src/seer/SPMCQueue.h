#pragma once

#include <concurrentqueue.h>
#include <thread>

namespace seer {

template <typename T>
class SPMCQueue {
    moodycamel::ConcurrentQueue<T> _queue;
    moodycamel::ProducerToken _producerToken;
    std::atomic<bool> _stopped = false;

public:
    SPMCQueue() : _producerToken(_queue) {}

    void enqueue(T* items, int size) {
        _queue.enqueue_bulk(_producerToken, std::make_move_iterator(items), size);
    }

    int dequeue(T* items, int max) {
        int size = 0;
        while (size = _queue.try_dequeue_bulk_from_producer(_producerToken, items, max), !size) {
            if (_stopped)
                return 0;
            std::this_thread::yield();
        }
        return size;
    }

    void stop() {
        _stopped = true;
    }
};

} // namespace seer
