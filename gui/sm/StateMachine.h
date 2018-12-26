#pragma once

#include <sml.hpp>
#include <string>

namespace gui {

    namespace sm {
        using namespace boost::sml;

        struct ParseEvent {};
        struct IndexEvent {};
        struct SearchEvent {
            std::string text;
            bool caseSensitive;
        };
        struct InterruptEvent {};
        struct FinishEvent {};
        struct FailEvent {};
        struct PausedEvent {};

        struct IStateHandler {
            virtual ~IStateHandler() = default;
            virtual void enterParsing() = 0;
            virtual void interruptParsing() = 0;
            virtual void enterIndexing() = 0;
            virtual void interruptIndexing() = 0;
            virtual void doneInterrupted() = 0;
            virtual void pauseAndSearch(SearchEvent) = 0;
            virtual void resumeIndexing() = 0;
            virtual void searchFromComplete(SearchEvent) = 0;
            virtual void enterFailed() = 0;
            virtual void enterComplete() = 0;
            virtual void enterInterrupted() = 0;
            virtual void searchFromPaused() = 0;
        };

        inline auto IdleState = state<class IdleState>;
        inline auto ParsingState = state<class ParsingState>;
        inline auto FailedState = state<class FailedState>;
        inline auto InterruptingState = state<class InterruptingState>;
        inline auto InterruptedState = state<class InterruptedState>;
        inline auto IndexingState = state<class IndexingState>;
        inline auto SearchingState = state<class SearchingState>;
        inline auto CompleteState = state<class CompleteState>;
        inline auto PausingIndexingState = state<class PausingIndexingState>;

        struct StateMachine {
            inline auto operator()() const {
#define ACTION(name) [](IStateHandler* h) { h->name(); }
#define ACTION_E(name) [](IStateHandler* h, const auto& event) { h->name(event); }

                // clang-format off
                return make_transition_table(
                    *IdleState + event<ParseEvent> / ACTION(enterParsing) = ParsingState,
                    ParsingState + event<InterruptEvent> / ACTION(interruptParsing) = InterruptingState,
                    ParsingState + event<IndexEvent> / ACTION(enterIndexing) = IndexingState,
                    ParsingState + event<FailEvent> / ACTION(enterFailed) = FailedState,
                    IndexingState + event<InterruptEvent> / ACTION(interruptIndexing) = InterruptingState,
                    IndexingState + event<FinishEvent> / ACTION(enterComplete) = CompleteState,
                    IndexingState + event<SearchEvent> / ACTION_E(pauseAndSearch) = PausingIndexingState,
                    IndexingState + event<FailEvent> / ACTION(enterFailed) = FailedState,
                    PausingIndexingState + event<PausedEvent> / ACTION(searchFromPaused) = SearchingState,
                    PausingIndexingState + event<FinishEvent> / ACTION(searchFromPaused) = SearchingState,
                    SearchingState + event<FinishEvent> / ACTION(resumeIndexing) = IndexingState,
                    SearchingState + event<FailEvent> / ACTION(enterFailed) = FailedState,
                    CompleteState + event<SearchEvent> / ACTION_E(searchFromComplete) = SearchingState,
                    CompleteState + event<InterruptEvent> / ACTION(enterInterrupted) = InterruptedState,
                    FailedState + event<InterruptEvent> / ACTION(enterInterrupted) = InterruptedState,
                    InterruptingState + event<FinishEvent> / ACTION(doneInterrupted) = InterruptedState);
                // clang-format on

#undef ACTION
#undef ACTION_E
            }
        };
    } // namespace sm
} // namespace gui
