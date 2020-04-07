#include "GraphemeMap.h"

#include <QTextBoundaryFinder>

gui::GraphemeMap::GraphemeMap(QString line) : _line(line) {
    int prevIndex = 0;
    int grapheme = 0;

    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, line);
    for (int index = finder.toNextBoundary(); index != -1; index = finder.toNextBoundary()) {
        int indexLen = index - prevIndex;
        int graphLen = 1;
        if (line[prevIndex] == '\t') {
            graphLen = seer::g_tabWidth - (grapheme % seer::g_tabWidth);
        }
        for (int i = 0; i < indexLen; ++i) {
            _indexToGrapheme.push_back(grapheme);
        }
        for (int i = 0; i < graphLen; ++i) {
            _graphemeToIndex.push_back(prevIndex);
        }
        prevIndex += indexLen;
        grapheme += graphLen;
    }

    finder = QTextBoundaryFinder(QTextBoundaryFinder::Word, line);
    int wordName = 0;
    prevIndex = 0;
    for (int index = finder.toNextBoundary(); index != -1; index = finder.toNextBoundary()) {
        int wordLen = index - prevIndex;

        auto gleft = std::get<0>(indexToGraphemeRange(prevIndex));
        auto gright = std::get<1>(indexToGraphemeRange(prevIndex + wordLen - 1));

        for (int i = 0; i < gright - gleft + 1; ++i) {
            _graphemeWords.push_back(wordName);
        }

        prevIndex += wordLen;
        wordName++;
    }
}

std::tuple<int, int> extendRange(const std::vector<int>& source, int left, int right) {
    assert(left <= (int)source.size()); // TODO: ssize
    assert(right <= (int)source.size()); // TODO: ssize

    auto leftValue = source[left];
    auto rightValue = source[right];

    while (left && source[left - 1] == leftValue) {
        --left;
    }
    while (right < (int)source.size() - 1 && source[right + 1] == rightValue) { // TODO: ssize
        ++right;
    }

    return {left, right};
}

std::tuple<int, int> gui::GraphemeMap::toVisibleRange(int left, int right, VisibleRangeType type) {
    if (left == -1 || right == -1)
        return {-1, -1};
    left = std::min(left, (int)_graphemeToIndex.size() - 1); // TODO: ssize
    right = std::min(right, (int)_graphemeToIndex.size() - 1); // TODO: ssize
    std::tie(left, right) = extendRange(_graphemeToIndex, left, right);
    if (type == VisibleRangeType::Word) {
        std::tie(left, right) = extendRange(_graphemeWords, left, right);
    }
    return {left, right};
}

int gui::GraphemeMap::graphemeSize() const {
    return _graphemeToIndex.size();
}

std::tuple<int, int> findRange(const std::vector<int>& source, int targetSize, int index) {
    assert(index < (int)source.size()); // TODO: ssize
    int left = source[index];
    if (index + 1 == (int)source.size()) { // TODO: ssize
        return {left, targetSize - 1};
    }
    return {left, std::max(left, source[index + 1] - 1)};
}

std::tuple<int, int> gui::GraphemeMap::indexToGraphemeRange(int index) const {
    return findRange(_indexToGrapheme, _graphemeToIndex.size(), index);
}

std::tuple<int, int> gui::GraphemeMap::graphemeToIndexRange(int grapheme) const {
    return findRange(_graphemeToIndex, _indexToGrapheme.size(), grapheme);
}

QString gui::GraphemeMap::graphemeRangeToString(int left, int right) const {
    QString str;
    for (int i = left; i <= right; ++i) {
        auto [ileft, iright] = graphemeToIndexRange(i);
        if (_line[ileft] == '\t') {
            str += " ";
        } else {
            str += _line.mid(ileft, iright - ileft + 1);
        }
    }
    return str;
}
