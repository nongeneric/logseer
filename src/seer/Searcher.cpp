#include "Searcher.h"
#include <assert.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace seer {

namespace {

template <class S>
struct Traits;

template <>
struct Traits<QString> {
    using CharPtr = PCRE2_SPTR16;
    using ReType = pcre2_code_16;
    using MatchData = pcre2_match_data_16;

    template <class... Args>
    static ReType* Compile(Args... args) {
        return pcre2_compile_16(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto JitCompile(Args... args) {
        return pcre2_jit_compile_16(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto CreateMatchDataFromPattern(Args... args) {
        return pcre2_match_data_create_from_pattern_16(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto MatchDataFree(Args... args) {
        return pcre2_match_data_free_16(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto JitMatch(Args... args) {
        return pcre2_jit_match_16(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto GetOvectorPointer(Args... args) {
        return pcre2_get_ovector_pointer_16(std::forward<Args>(args)...);
    }
};

template <>
struct Traits<std::string> {
    using CharPtr = PCRE2_SPTR8;
    using ReType = pcre2_code_8;
    using MatchData = pcre2_match_data_8;

    template <class... Args>
    static ReType* Compile(Args... args) {
        return pcre2_compile_8(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto JitCompile(Args... args) {
        return pcre2_jit_compile_8(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto CreateMatchDataFromPattern(Args... args) {
        return pcre2_match_data_create_from_pattern_8(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto MatchDataFree(Args... args) {
        return pcre2_match_data_free_8(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto JitMatch(Args... args) {
        return pcre2_jit_match_8(std::forward<Args>(args)...);
    }

    template <class... Args>
    static auto GetOvectorPointer(Args... args) {
        return pcre2_get_ovector_pointer_8(std::forward<Args>(args)...);
    }
};

template <class S>
class RegexSearcher {
    typename Traits<S>::ReType* _re;
    std::shared_ptr<typename Traits<S>::MatchData> _matchData;

public:
    RegexSearcher(const S& pattern, bool regex, bool caseSensitive, bool unicodeAware) {
        int reError;
        PCRE2_SIZE errorOffset;
        const auto flags = (regex ? 0 : PCRE2_LITERAL)
                         | (unicodeAware ? PCRE2_UTF : 0)
                         | (caseSensitive ? 0 : PCRE2_CASELESS);
        _re = Traits<S>::Compile(reinterpret_cast<typename Traits<S>::CharPtr>(pattern.data()),
                                 PCRE2_ZERO_TERMINATED,
                                 flags,
                                 &reError,
                                 &errorOffset,
                                 nullptr);
        if (!_re)
            return;

        reError = Traits<S>::JitCompile(_re, PCRE2_JIT_COMPLETE);
        assert(reError == 0);
        _matchData = std::shared_ptr<typename Traits<S>::MatchData>(
            Traits<S>::CreateMatchDataFromPattern(_re, nullptr),
            [] (auto ptr) { Traits<S>::MatchDataFree(ptr); });
    }

    std::tuple<int, int> search(const S& text, size_t start) {
        if (start >= static_cast<size_t>(text.size()))
            return {-1, -1};

        if (!_re)
            return {-1,-1};

        auto rc = Traits<S>::JitMatch(_re,
                                      reinterpret_cast<typename Traits<S>::CharPtr>(text.data()),
                                      text.size(),
                                      start,
                                      0,
                                      _matchData.get(),
                                      nullptr);

        if (rc < 0)
            return {-1, -1};

        auto vec = Traits<S>::GetOvectorPointer(_matchData.get());
        if (vec[0] >= static_cast<size_t>(text.size()))
            return {-1, -1};

        return {vec[0], vec[1] - vec[0]};
    }
};

class Searcher : public ISearcher {
    RegexSearcher<std::string> _impl;

public:
    Searcher(const std::string& pattern, bool regex, bool caseSensitive, bool unicodeAware)
        : _impl(pattern, regex, caseSensitive, unicodeAware) {}

    std::tuple<int, int> search(std::string const& text, size_t start) override {
        return _impl.search(text, start);
    }
};

class HighlightSearcher : public IHighlightSearcher {
    RegexSearcher<QString> _impl;

public:
    HighlightSearcher(QString pattern, bool regex, bool caseSensitive, bool unicodeAware)
        : _impl(pattern, regex, caseSensitive, unicodeAware) {}

    std::tuple<int, int> search(QString text, size_t start) override {
        return _impl.search(text, start);
    }
};

} // namespace

std::unique_ptr<ISearcher> createSearcher(std::string const& pattern,
                                          bool regex,
                                          bool caseSensitive,
                                          bool unicodeAware) {
    return std::make_unique<Searcher>(pattern, regex, caseSensitive, unicodeAware);
}

std::unique_ptr<IHighlightSearcher> createHighlightSearcher(QString pattern,
                                                            bool regex,
                                                            bool caseSensitive,
                                                            bool unicodeAware) {
    return std::make_unique<HighlightSearcher>(pattern, regex, caseSensitive, unicodeAware);
}

} // namespace seer
