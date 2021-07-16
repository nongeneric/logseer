#pragma once

#include <QPushButton>
#include <QLineEdit>
#include <QDialog>
#include <memory>
#include <optional>

namespace gui {

class GotoLineDialog : public QDialog {
    Q_OBJECT

    QLineEdit* _line;
    QPushButton* _go;
    QPushButton* _cancel;
    int64_t _lineCount;

public:
    explicit GotoLineDialog(QWidget* parent, int64_t lineCount);
    std::optional<int64_t> getLine() const;
};

} // namespace gui
