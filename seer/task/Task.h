#pragma once

#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace seer::task {

enum class TaskState {
    Idle, Running, Paused, Finished, Failed, Stopped
};

class Task {
    std::function<void(TaskState)> _stateChanged;
    std::function<void(int)> _progressChanged;
    std::atomic<bool> _isStopRequested = false;
    std::atomic<bool> _isPauseRequested = false;
    std::thread _thread;
    TaskState _state = TaskState::Idle;
    std::atomic<int> _progress = -1;
    std::recursive_mutex _mState;
    std::condition_variable_any _cvPause;
    void changeState(TaskState state);

protected:
    virtual void body() = 0;
    void waitPause();
    void reportProgress(int progress);
    void reportError();
    void reportStopped();
    bool isStopRequested();

public:
    void setStateChanged(std::function<void(TaskState)> handler);
    void setProgressChanged(std::function<void(int)> handler);
    void start();
    void stop();
    void pause();
    virtual ~Task();
};

} // namespace seer
