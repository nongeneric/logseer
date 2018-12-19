#include "Task.h"
#include <assert.h>

namespace seer {

    void Task::changeState(TaskState state) {
        auto lock = std::lock_guard(_mState);
        _cvPause.notify_all();
        _state = state;
        if (_stateChanged)
            _stateChanged(state);
    }

    void Task::waitPause() {
        auto lock = std::unique_lock(_mState);
        if (_isPauseRequested) {
            changeState(TaskState::Paused);
            _isPauseRequested = false;
        }
        _cvPause.wait(lock, [=] { return _state != TaskState::Paused; });
    }

    void Task::reportProgress(int progress) {
        assert(0 <= progress && progress <= 100);
        if (_progress == progress)
            return;
        _progress = progress;
        if (_progressChanged)
            _progressChanged(progress);
    }

    void Task::reportError() {
        changeState(TaskState::Failed);
    }

    bool Task::isStopRequested() {
        return _isStopRequested;
    }

    void Task::setStateChanged(std::function<void(TaskState)> handler) {
        _stateChanged = handler;
    }

    void Task::setProgressChanged(std::function<void(int)> handler) {
        _progressChanged = handler;
    }

    void Task::start() {
        changeState(TaskState::Running);
        if (_thread.joinable())
            return;
        _thread = std::thread([=] {
            body();
            if (_state != TaskState::Failed)
                changeState(TaskState::Finished);
        });
    }

    void Task::stop() {
        _isStopRequested = true;
    }

    void Task::pause() {
        _isPauseRequested = true;
    }

    Task::~Task() {
        _thread.join();
    }

} // namespace seer
