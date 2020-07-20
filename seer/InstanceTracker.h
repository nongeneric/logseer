#pragma once

#include <blockingconcurrentqueue.h>

#include <functional>
#include <optional>
#include <string>
#include <thread>

namespace seer {

class InstanceTracker {
    std::thread _thread;
    int _socket = -1;
    bool _connected = false;
    moodycamel::BlockingConcurrentQueue<std::optional<std::string>> _queue;

public:
    InstanceTracker(std::string socketName);
    bool connected() const;
    void send(std::string message);
    void stop();
    std::optional<std::string> waitMessage();
};

}
