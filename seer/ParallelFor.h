#pragma once

#include <blockingconcurrentqueue.h>
#include <thread>
#include <optional>
#include <vector>
#include <type_traits>

namespace seer {

    template <typename C, typename F>
    void parallelFor(const C& container, F action) {
        using T = std::remove_cv_t<std::remove_reference_t<decltype(*begin(container))>>;
        moodycamel::BlockingConcurrentQueue<std::optional<T>> queue;

        auto threadCount = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(threadCount);

        for (auto& th : threads) {
            th = std::thread([&] {
                std::optional<T> item;
                for (;;) {
                    queue.wait_dequeue(item);
                    if (!item)
                        return;
                    action(*item);
                }
            });
        }

        for (auto& i : container) {
            queue.enqueue(i);
        }

        for ([[maybe_unused]] auto& th : threads) {
            queue.enqueue({});
        }

        for (auto& th : threads) {
            th.join();
        }
    }

} // namespace seer
