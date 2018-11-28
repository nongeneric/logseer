#pragma once

#include <QWidget>
#include <string>

namespace gui {

    class SearchLine : public QWidget {
        Q_OBJECT

    public:
        explicit SearchLine(QWidget* parent = nullptr);

    signals:
        void requestSearch(std::string string);
    };

} // namespace gui
