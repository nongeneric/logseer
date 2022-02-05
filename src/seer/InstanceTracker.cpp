#include "InstanceTracker.h"

#include <seer/Log.h>
#include <boost/filesystem.hpp>
#include <assert.h>
#include <unistd.h>

namespace seer {

InstanceTracker::InstanceTracker(std::string socketName) {
    LOGSEER_SOCKET_INIT();
    socketName = (boost::filesystem::temp_directory_path() / socketName).string();

    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == LOGSEER_INVALID_SOCKET) {
        log_infof("{}: failed to create AF_UNIX socket, error {}", __func__, LOGSEER_ERRNO);
        return;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketName.c_str(), sizeof(addr.sun_path) - 1);

    if (!connect(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr)) {
        log_infof("{}: connected to socket {}", __func__, socketName);
        _connected = true;
        return;
    }

#if __MINGW32__
    LOGSEER_CLOSE_SOCKET(_socket);
    _socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (_socket == LOGSEER_INVALID_SOCKET) {
        log_infof("{}: failed to create AF_UNIX socket after failed connect(), error {}", __func__, LOGSEER_ERRNO);
        return;
    }
#endif

    LOGSEER_UNLINK(socketName.c_str());
    auto ret = bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    if (ret == -1) {
        log_infof("{}: failed to bind to socket {}, error {}", __func__, socketName, LOGSEER_ERRNO);
        stop();
        return;
    }

    ret = listen(_socket, 20);
    if (ret == -1) {
        log_infof("{}: failed to listen on socket {}", __func__, socketName);
        stop();
        return;
    }

    log_infof("{}: listening on socket {}", __func__, socketName);

    _thread = std::thread([this] {
        for (;;) {
            auto data = accept(_socket, NULL, NULL);
            if (data == LOGSEER_INVALID_SOCKET) {
                _queue.enqueue({});
                break;
            }

            std::string message;

            for (;;) {
                char ch = 0;
                auto ret = LOGSEER_SOCKET_READ(data, &ch, 1);
                if (ret == 0)
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

void InstanceTracker::send(std::string path) {
    assert(connected());
    auto message = boost::filesystem::absolute(path).string() + '\n';
    auto ret = LOGSEER_SOCKET_WRITE(_socket, message.c_str(), message.size() + 1);
    if (ret == 0) {
        log_infof("{}: could not write to socket, error {}", __func__, LOGSEER_ERRNO);
        return;
    }
}

void InstanceTracker::stop() {
    if (_socket != LOGSEER_INVALID_SOCKET) {
        shutdown(_socket, LOGSEER_SHUTDOWN_RDWR);
        LOGSEER_CLOSE_SOCKET(_socket);
        _socket = LOGSEER_INVALID_SOCKET;
    }

    _queue.enqueue({});

    if (_thread.joinable()) {
        _thread.join();
    }
}

std::optional<std::string> InstanceTracker::waitMessage() {
    std::optional<std::string> message;
    _queue.wait_dequeue(message);
    return message;
}

}
