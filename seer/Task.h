#pragma once

#include <functional>
#include <atomic>
#include <thread>

namespace seer {

    enum class TaskState {
        Idle, Running, Finished, Failed
    };

    class Task {
        std::function<void(TaskState)> _stateChanged;
        std::function<void(int)> _progressChanged;
        std::atomic<bool> _isStopRequested = false;
        std::thread _thread;
        TaskState _state = TaskState::Idle;
        std::atomic<int> _progress = -1;
        void changeState(TaskState state);

    protected:
        virtual void body() = 0;
        void reportProgress(int progress);
        void reportError();
        bool isStopRequested();

    public:
        void setStateChanged(std::function<void(TaskState)> handler);
        void setProgressChanged(std::function<void(int)> handler);
        void start();
        void stop();
        virtual ~Task();
    };

} // namespace seer
