#include "InstanceTracker.h"

#ifndef __MINGW32__
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>
#endif

namespace seer {

#ifdef __MINGW32__
InstanceTracker::InstanceTracker(std::string) {}
InstanceTracker::~InstanceTracker() {}
bool InstanceTracker::connected() const { return false; }
void InstanceTracker::send(std::string) {}
void InstanceTracker::stop() {}
std::optional<std::string> waitMessage() { return {}; }
#else
InstanceTracker::InstanceTracker(std::string socketName) {
    socketName = (boost::filesystem::temp_directory_path() / socketName).string();

    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == -1) {
        return;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketName.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != -1) {
        _connected = true;
        return;
    }

    unlink(socketName.c_str());
    auto ret = bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    if (ret == -1) {
        return;
    }

    ret = listen(_socket, 20);
    if (ret == -1) {
        return;
    }

    _thread = std::thread([=] {
        for (;;) {
            auto data = accept(_socket, NULL, NULL);
            if (data == -1) {
                _queue.enqueue({});
                break;
            }

            std::string message;

            for (;;) {
                char ch = 0;
                auto ret = read(data, &ch, 1);
                if (ret == -1)
                    break;

                if (ch == '\n')
                    break;

                message += ch;
            }

            close(data);
            _queue.enqueue(message);
        }
    });
}

bool InstanceTracker::connected() const {
    return _connected;
}

void InstanceTracker::send(std::string message) {
    assert(connected());
    message += '\n';
    auto ret = write(_socket, message.c_str(), message.size() + 1);
    if (ret == -1) {
        return;
    }
}

void InstanceTracker::stop() {
    if (_socket != -1) {
        shutdown(_socket, SHUT_RDWR);
    }

    if (_thread.joinable()) {
        _thread.join();
    }

    if (_socket != -1) {
        close(_socket);
    }
}

std::optional<std::string> InstanceTracker::waitMessage() {
    std::optional<std::string> message;
    _queue.wait_dequeue(message);
    return message;
}
#endif

}
