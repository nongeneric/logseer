#pragma once

#include <QWidget>
#include <string>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

namespace gui {

    class SearchLine : public QWidget {
        Q_OBJECT

        QLabel* _status;
        QProgressBar* _progress;
        QPushButton* _button;

    public:
        explicit SearchLine(bool caseSensitive, QWidget* parent = nullptr);
        void setStatus(std::string status);
        void setProgress(int progress);
        void setSearchEnabled(bool enabled);

    signals:
        void searchRequested(std::string string, bool caseSensitive);
        void caseSensitiveChanged(bool caseSensitive);
    };

} // namespace gui
