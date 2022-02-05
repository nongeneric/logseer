#pragma once

#include <QWidget>
#include <string>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

namespace gui {

enum class SearchButtonTitle {
    Search,
    Abort
};

class SearchLine : public QWidget {
    Q_OBJECT

    QLabel* _status;
    QProgressBar* _progress;
    QPushButton* _button;

public:
    SearchLine(bool regexInitial,
               bool caseSensitiveInitial,
               bool messageOnlyInitial,
               bool unicodeAwareInitial,
               QWidget* parent = nullptr);
    void setStatus(std::string status);
    void setSearchButtonTitle(SearchButtonTitle title);
    void setProgress(int progress);
    void setSearchEnabled(bool enabled);

signals:
    void searchRequested(std::string string,
                         bool regex,
                         bool caseSensitive,
                         bool unicodeAware,
                         bool messageOnly);
    void regexChanged(bool regex);
    void caseSensitiveChanged(bool caseSensitive);
    void messageOnlyChanged(bool messageOnly);
};

} // namespace gui
