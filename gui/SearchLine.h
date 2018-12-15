#pragma once

#include <QWidget>
#include <string>
#include <QProgressBar>
#include <QLabel>

namespace gui {

    class SearchLine : public QWidget {
        Q_OBJECT

        QLabel* _status;
        QProgressBar* _progress;

    public:
        explicit SearchLine(QWidget* parent = nullptr);
        void setStatus(std::string status);
        void setProgress(int progress);

    signals:
        void searchRequested(std::string string, bool caseSensitive);
    };

} // namespace gui
