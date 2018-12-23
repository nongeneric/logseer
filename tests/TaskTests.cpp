#include <catch2/catch.hpp>

#include "seer/task/Task.h"
#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

class TestTask : public seer::task::Task {
protected:
    void body() override {
        reportProgress(0);
        reportProgress(20);
        reportProgress(20);
        reportProgress(30);
        reportProgress(30);
        reportProgress(30);
        reportProgress(80);
    }
};

TEST_CASE("collapse_repeated_progress_states") {
    TestTask task;
    std::atomic<int> timestamp = 0;
    std::vector<std::string> events;
    std::mutex m;
    std::condition_variable cv;

    bool finished = false;

    task.setStateChanged([&](auto state) {
        auto stateStr = state == seer::task::TaskState::Idle ? "Idle"
                      : state == seer::task::TaskState::Running ? "Running"
                      : state == seer::task::TaskState::Failed ? "Failed"
                      : state == seer::task::TaskState::Finished ? "Finished"
                      : "Unknown";
        auto str = std::to_string(timestamp++) + " " + stateStr;
        auto lock = std::lock_guard(m);
        events.push_back(str);
        finished = state == seer::task::TaskState::Finished;
        cv.notify_all();
    });

    task.setProgressChanged([&] (auto progress) {
        auto str = std::to_string(timestamp++) + " " + std::to_string(progress);
        auto lock = std::lock_guard(m);
        events.push_back(str);
    });

    task.start();

    auto lock = std::unique_lock(m);
    cv.wait(lock, [&] { return finished; });

    auto expected = std::vector<std::string> {
        "0 Running", "1 0", "2 20", "3 30", "4 80", "5 Finished"
    };

    REQUIRE( events == expected );
}
