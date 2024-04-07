#pragma once

namespace gui {

struct RowColumn {
    int row = 0;
    int column = 0;

    bool operator==(RowColumn const& other) const = default;
};

struct RowColumnHash {
    int operator()(const RowColumn& rowColumn) const {
        return rowColumn.row ^ rowColumn.column;
    }
};

} // namespace gui