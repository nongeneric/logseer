#pragma once

#include <blockingconcurrentqueue.h>
#include "sockets.h"

#include <functional>
#include <optional>
#include <string>
#include <thread>

namespace seer {

class InstanceTracker {
    std::thread _thread;
    LOGSEER_SOCKET_TYPE _socket = LOGSEER_INVALID_SOCKET;
    bool _connected = false;
    moodycamel::BlockingConcurrentQueue<std::optional<std::string>> _queue;

public:
    InstanceTracker(std::string socketName);
    bool connected() const;
    void send(std::string path);
    void stop();
    std::optional<std::string> waitMessage();
};

}
