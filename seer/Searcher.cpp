#include "Searcher.h"
#include <QString>

#define PCRE2_CODE_UNIT_WIDTH 16
#include <pcre2.h>

namespace seer {

    namespace {

        class RegexSearcher : public ISearcher {
            QString _pattern;
            pcre2_code* _re;
            std::shared_ptr<pcre2_match_data> _matchData;

        public:
            RegexSearcher(QString const& pattern, bool caseSensitive) {
                int reError;
                PCRE2_SIZE errorOffset;
                _re = pcre2_compile((PCRE2_SPTR16)pattern.data(),
                                    PCRE2_ZERO_TERMINATED,
                                    caseSensitive ? 0 : PCRE2_CASELESS,
                                    &reError,
                                    &errorOffset,
                                    nullptr);
                if (!_re)
                    return;

                reError = pcre2_jit_compile(_re, PCRE2_JIT_COMPLETE);
                assert(reError == 0);
                _matchData = std::shared_ptr<pcre2_match_data>(
                    pcre2_match_data_create_from_pattern(_re, nullptr),
                    pcre2_match_data_free);
            }

            std::tuple<int, int> search(const QString &text, int start) override {
                if (!_re)
                    return {-1,-1};

                auto rc = pcre2_jit_match(_re,
                                          (PCRE2_SPTR16)text.data(),
                                          text.size(),
                                          start,
                                          0,
                                          _matchData.get(),
                                          nullptr);
                if (rc < 0)
                    return {-1,-1};

                auto vec = pcre2_get_ovector_pointer(_matchData.get());
                return {vec[0], vec[1] - vec[0]};
            }
        };

        class SimpleSearcher : public ISearcher {
            QString _pattern;
            Qt::CaseSensitivity _caseSensitive;

        public:
            SimpleSearcher(QString const& pattern, bool caseSensitive) {
                _pattern = pattern;
                _caseSensitive =
                    caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
            }

            std::tuple<int, int> search(QString const& text, int start) override {
                auto index = text.indexOf(_pattern, start, _caseSensitive);
                return {index, _pattern.length()};
            }
        };

    } // namespace

    std::unique_ptr<ISearcher> createSearcher(QString const& pattern,
                                              bool regex,
                                              bool caseSensitive) {
        if (regex) {
            return std::make_unique<RegexSearcher>(pattern, caseSensitive);
        } else {
            return std::make_unique<SimpleSearcher>(pattern, caseSensitive);
        }
    }

} // namespace seer
