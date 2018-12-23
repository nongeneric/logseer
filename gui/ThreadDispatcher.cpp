#include "ThreadDispatcher.h"
#include <mutex>

namespace gui {

    namespace {
        std::once_flag flag;
    }

    ThreadDispatcher::ThreadDispatcher(QObject* parent) : QObject(parent) {
        std::call_once(flag, [=] {
            qRegisterMetaType<std::function<void()>>("std::function<void()>");
        });
        connect(this,
                &ThreadDispatcher::postToUIThreadSignal,
                this,
                [=](auto action) { action(); },
                Qt::QueuedConnection);
    }

    void ThreadDispatcher::postToUIThread(std::function<void()> action) {
        emit postToUIThreadSignal(action);
    }

} // namespace gui
