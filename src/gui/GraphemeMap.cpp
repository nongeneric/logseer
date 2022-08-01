#include "GraphemeMap.h"

#include <cmath>

#include <QTextBoundaryFinder>

namespace gui {

GraphemeMap::GraphemeMap(QString line, IFontMetrics* metrics) : _line(line) {
    if (line.size() == 0)
        return;

    auto getWidth = [&] (auto str) {
        if (metrics)
            return metrics->width(str);
        return 0.f;
    };

    float position = 0;
    int prevIndex = 0;
    int grapheme = 0;

    auto tabWidth = getWidth(" ") * seer::g_tabWidth;

    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, line);
    for (int index = finder.toNextBoundary(); index != -1; index = finder.toNextBoundary()) {
        int indexLen = index - prevIndex;
        for (int i = 0; i < indexLen; ++i) {
            _indexToGrapheme.push_back(grapheme);
        }
        _graphemeToIndex.push_back(prevIndex);
        _graphemePositions.push_back(position);

        if (line[prevIndex] == '\t') {
            position += tabWidth - std::fmod(position, tabWidth);
        } else {
            position += getWidth(line.mid(prevIndex, indexLen));
        }

        prevIndex += indexLen;
        grapheme++;
    }

    _graphemePositions.push_back(position);

    finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, line);
    int wordName = 0;
    prevIndex = 0;
    for (int index = finder.toNextBoundary(); index != -1; index = finder.toNextBoundary()) {
        int wordLen = index - prevIndex;

        auto gleft = indexToGrapheme(prevIndex);
        auto gright = indexToGrapheme(prevIndex + wordLen - 1);

        for (int i = 0; i < gright - gleft + 1; ++i) {
            _graphemeWords.push_back(wordName);
        }

        prevIndex += wordLen;
        wordName++;
    }
}

std::tuple<int, int> GraphemeMap::extendToWordBoundary(int left, int right) const {
    if (left == -1 || right == -1 || _graphemeToIndex.empty())
        return {-1, -1};

    left = std::min<int>(left, std::ssize(_graphemeToIndex) - 1);
    right = std::min<int>(right, std::ssize(_graphemeToIndex) - 1);

    auto leftValue = _graphemeWords[left];
    auto rightValue = _graphemeWords[right];

    while (left && _graphemeWords[left - 1] == leftValue) {
        --left;
    }
    while (right < std::ssize(_graphemeWords) - 1 &&
           _graphemeWords[right + 1] == rightValue) {
        ++right;
    }

    return {left, right};
}

float GraphemeMap::getPosition(int grapheme) const {
    if (grapheme == -1 || _graphemePositions.empty())
        return 0;
    grapheme = std::min<int>(grapheme, std::ssize(_graphemePositions) - 1);
    return _graphemePositions[grapheme];
}

int GraphemeMap::findGrapheme(float position) const {
    if (_graphemePositions.empty())
        return -1;

    auto it = std::lower_bound(begin(_graphemePositions), end(_graphemePositions), position);
    if (it == begin(_graphemePositions))
        return 0;
    if (it == end(_graphemePositions))
        return _graphemePositions.size() - 1;

    return std::distance(begin(_graphemePositions), it) - 1;
}

int GraphemeMap::graphemeSize() const {
    return _graphemeToIndex.size();
}

int GraphemeMap::indexToGrapheme(int index) const {
    assert(index < std::ssize(_indexToGrapheme));
    return _indexToGrapheme[index];
}

std::tuple<int, int> GraphemeMap::graphemeToIndexRange(int grapheme) const {
    assert(grapheme < std::ssize(_graphemeToIndex));
    auto left = _graphemeToIndex[grapheme];
    if (grapheme + 1 == std::ssize(_graphemeToIndex)) {
        return {left, _indexToGrapheme.size() - 1};
    }
    return {left, std::max(left, _graphemeToIndex[grapheme + 1] - 1)};
}

const QString& GraphemeMap::line() const {
    return _line;
}

int GraphemeMap::pixelWidth() const {
    if (_graphemePositions.empty())
        return 0;
    return _graphemePositions.back();
}

}
