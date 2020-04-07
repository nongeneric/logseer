#pragma once

#include <seer/Index.h>

#include <QString>
#include <vector>

namespace gui {

enum class VisibleRangeType {
    Grapheme,
    Word
};

class GraphemeMap {
    std::vector<int> _indexToGrapheme;
    std::vector<int> _graphemeToIndex;
    std::vector<int> _graphemeWords;
    QString _line;

public:
    GraphemeMap(QString line);
    std::tuple<int, int> toVisibleRange(int left, int right, VisibleRangeType type = VisibleRangeType::Grapheme);
    int graphemeSize() const;
    std::tuple<int, int> indexToGraphemeRange(int index) const;
    std::tuple<int, int> graphemeToIndexRange(int grapheme) const;
    QString graphemeRangeToString(int left, int right) const;
};

} // namespace gui
