#pragma once

#include <sml.hpp>
#include <string>

namespace gui::sm {

using namespace boost::sml;

struct IndexEvent {};
struct SearchEvent {
    std::string text;
    bool regex;
    bool caseSensitive;
    bool messageOnly;
};
struct InterruptEvent {};
struct ReloadEvent {
    std::shared_ptr<std::istream> stream;
    std::shared_ptr<seer::ILineParser> parser;
};
struct FinishEvent {};
struct FailEvent {};

struct IStateHandler {
    virtual ~IStateHandler() = default;
    virtual void enterIndexing() = 0;
    virtual void interruptIndexing() = 0;
    virtual void resumeIndexing() = 0;
    virtual void searchFromComplete(SearchEvent) = 0;
    virtual void enterFailed() = 0;
    virtual void enterComplete() = 0;
    virtual void enterInterrupted() = 0;
    virtual void searchFromSearching(SearchEvent) = 0;
};

inline auto IdleState = state<class IdleState>;
inline auto FailedState = state<class FailedState>;
inline auto InterruptingState = state<class InterruptingState>;
inline auto InterruptedState = state<class InterruptedState>;
inline auto IndexingState = state<class IndexingState>;
inline auto SearchingState = state<class SearchingState>;
inline auto CompleteState = state<class CompleteState>;

#define MAKE_ACTION(name) \
    struct a_ ## name { \
        auto operator()(IStateHandler* h) { \
            h->name(); \
        } \
    }

#define MAKE_ACTION_E(name) \
    struct a_ ## name { \
        template <class Event> \
        auto operator()(IStateHandler* h, const Event& event) { \
            h->name(event); \
        } \
    }

struct StateMachine {
    MAKE_ACTION(enterIndexing);
    MAKE_ACTION(interruptIndexing);
    MAKE_ACTION(resumeIndexing);
    MAKE_ACTION_E(searchFromComplete);
    MAKE_ACTION(enterFailed);
    MAKE_ACTION(enterComplete);
    MAKE_ACTION(enterInterrupted);
    MAKE_ACTION_E(searchFromSearching);

    inline auto operator()() const {
#define ACTION(name) a_ ## name {}

        // clang-format off
        return make_transition_table(
            *IdleState + event<IndexEvent> / ACTION(enterIndexing) = IndexingState,
            IndexingState + event<InterruptEvent> / ACTION(interruptIndexing) = InterruptingState,
            IndexingState + event<FinishEvent> / ACTION(enterComplete) = CompleteState,
            IndexingState + event<FailEvent> / ACTION(enterFailed) = FailedState,
            SearchingState + event<FinishEvent> / ACTION(resumeIndexing) = IndexingState,
            SearchingState + event<SearchEvent> / ACTION(searchFromSearching) = SearchingState,
            SearchingState + event<FailEvent> / ACTION(enterFailed) = FailedState,
            CompleteState + event<SearchEvent> / ACTION(searchFromComplete) = SearchingState,
            CompleteState + event<InterruptEvent> / ACTION(enterInterrupted) = InterruptedState,
            FailedState + event<InterruptEvent> / ACTION(enterInterrupted) = InterruptedState,
            InterruptingState + event<FinishEvent> / ACTION(enterInterrupted) = InterruptedState,
            InterruptingState + event<IndexEvent> / ACTION(enterInterrupted) = InterruptedState,
            InterruptedState + event<IndexEvent> / ACTION(enterIndexing) = IndexingState);
        // clang-format on

#undef ACTION
#undef MAKE_ACTION
#undef MAKE_ACTION_E
    }
};

} // namespace gui::sm
