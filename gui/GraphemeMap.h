#pragma once

#include <seer/Index.h>

#include <QString>
#include <vector>

namespace gui {

struct IFontMetrics {
    virtual ~IFontMetrics() = default;
    virtual float width(const QString& str) = 0;
};

class GraphemeMap {
    std::vector<int> _indexToGrapheme;
    std::vector<int> _graphemeToIndex;
    std::vector<int> _graphemeWords;
    std::vector<float> _graphemePositions;
    QString _line;

public:
    GraphemeMap(QString line, IFontMetrics* metrics);
    std::tuple<int, int> extendToWordBoundary(int left, int right) const;
    float getPosition(int grapheme) const;
    int findGrapheme(float position) const;
    int graphemeSize() const;
    int indexToGrapheme(int index) const;
    std::tuple<int, int> graphemeToIndexRange(int grapheme) const;
    const QString& line() const;
};

} // namespace gui
