#pragma once

#include <QString>
#include <string>
#include <memory>
#include <tuple>

namespace seer {

    struct ISearcher {
        virtual std::tuple<int, int> search(QString const& text, int start) = 0;
        virtual ~ISearcher() = default;
    };

    std::unique_ptr<ISearcher> createSearcher(QString const& pattern,
                                              bool regex,
                                              bool caseSensitive);

} // namespace seer
