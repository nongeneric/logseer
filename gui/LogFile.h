#pragma once

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "ThreadDispatcher.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include "seer/task/ParsingTask.h"
#include "seer/task/IndexingTask.h"
#include "seer/task/SearchingTask.h"
#include "seer/Log.h"
#include "sm/Logger.h"
#include "sm/StateMachine.h"
#include <sml.hpp>
#include <QObject>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <atomic>
#include <istream>
#include <optional>

namespace gui {

    class LogFile : public QObject, sm::IStateHandler {
        Q_OBJECT
        std::unique_ptr<seer::FileParser> _fileParser;
        std::shared_ptr<seer::ILineParser> _lineParser;
        std::unique_ptr<seer::Index> _index;
        std::shared_ptr<seer::Index> _searchIndex;
        std::unique_ptr<LogTableModel> _logTableModel;
        std::unique_ptr<LogTableModel> _searchLogTableModel;
        std::map<int, std::set<std::string>> _columnFilters;
        std::unique_ptr<std::istream> _stream;
        std::shared_ptr<seer::ILineParserRepository> _repository;
        std::unique_ptr<seer::task::Task> _parsingTask;
        std::unique_ptr<seer::task::Task> _indexingTask;
        std::unique_ptr<seer::task::SearchingTask> _searchingTask;
        sm::Logger _smLogger;
        boost::sml::sm<sm::StateMachine, boost::sml::logger<sm::Logger>> _sm;
        ThreadDispatcher _dispatcher;
        std::optional<sm::SearchEvent> _scheduledSearchEvent;
        bool _indexingComplete = false;

        void enterParsing() override;
        void interruptParsing() override;
        void enterIndexing() override;
        void interruptIndexing() override;
        void doneInterrupted() override;
        void pauseAndSearch(sm::SearchEvent) override;
        void resumeIndexing() override;
        void searchFromComplete(sm::SearchEvent) override;
        void enterFailed() override;
        void enterComplete() override;
        void enterInterrupted() override;
        void searchFromPaused() override;

        inline void finish() {
            _sm.process_event(sm::FinishEvent{});
        }

        inline void paused() {
            _sm.process_event(sm::PausedEvent{});
        }

        inline void fail() {
            _sm.process_event(sm::FailEvent{});
        }

    protected:
        virtual seer::task::Task* createIndexingTask(
            seer::Index* index,
            seer::FileParser* fileParser,
            seer::ILineParser* lineParser);

        virtual seer::task::Task* createParsingTask(seer::FileParser* fileParser);

    public:
        LogFile(std::unique_ptr<std::istream>&& stream,
                std::shared_ptr<seer::ILineParserRepository> repository);
        ~LogFile() = default;

        inline void parse() {
            _sm.process_event(sm::ParseEvent{});
        }

        inline void index() {
            _sm.process_event(sm::IndexEvent{});
        }

        inline void search(std::string text, bool caseSensitive) {
            _sm.process_event(sm::SearchEvent{text, caseSensitive});
        }

        inline void interrupt() {
            _sm.process_event(sm::InterruptEvent{});
        }

        void requestFilter(int column);
        void setColumnFilter(int column, std::set<std::string> values);
        LogTableModel* logTableModel();
        LogTableModel *searchLogTableModel();

        inline bool isState(auto state) {
            return _sm.is(state);
        }

    signals:
        void filterRequested(FilterTableModel* model, int column);
        void stateChanged();
        void progressChanged(int progress);
    };

} // namespace gui
