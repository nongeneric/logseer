#include "Task.h"
#include <assert.h>

namespace seer {

    void Task::changeState(TaskState state) {
        if (_stateChanged)
            _stateChanged(state);
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
        _thread = std::thread([=] {
            body();
            if (_state != TaskState::Failed)
                changeState(TaskState::Finished);
        });
    }

    void Task::stop() {
        _isStopRequested = true;
    }

    Task::~Task() {
        _thread.join();
    }

} // namespace seer
