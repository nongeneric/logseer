#pragma once

#include <QString>
#include <string>
#include <memory>
#include <tuple>

namespace seer {

struct ISearcher {
    virtual std::tuple<int, int> search(std::string const& text, size_t start) = 0;
    virtual ~ISearcher() = default;
};

struct IHighlightSearcher {
    virtual std::tuple<int, int> search(QString text, size_t start) = 0;
    virtual ~IHighlightSearcher() = default;
};

std::unique_ptr<ISearcher> createSearcher(std::string const& pattern,
                                          bool regex,
                                          bool caseSensitive,
                                          bool unicodeAware);

std::unique_ptr<IHighlightSearcher> createHighlightSearcher(QString pattern,
                                                            bool regex,
                                                            bool caseSensitive,
                                                            bool unicodeAware);

} // namespace seer
