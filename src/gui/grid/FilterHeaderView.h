#pragma once

#include <QHeaderView>
#include <QPoint>

namespace gui::grid {

class LogTable;

class FilterHeaderView : public QHeaderView {
    LogTable* _table;

public:
    FilterHeaderView(LogTable* table);
    LogTable* logTable() const;
};

} // namespace gui
