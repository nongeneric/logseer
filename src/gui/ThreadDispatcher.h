#pragma once
#include <QObject>
#include <functional>

namespace gui {

class ThreadDispatcher : public QObject {
    Q_OBJECT
public:
    explicit ThreadDispatcher(QObject* parent = nullptr);
    void postToUIThread(std::function<void()> action);

signals:
    void postToUIThreadSignal(std::function<void()> action);
};

} // namespace gui
