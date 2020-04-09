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
#include "seer/Hist.h"
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
    std::shared_ptr<std::istream> _stream;
    std::unique_ptr<seer::task::Task> _parsingTask;
    std::unique_ptr<seer::task::Task> _indexingTask;
    std::unique_ptr<seer::task::SearchingTask> _searchingTask;
    std::shared_ptr<seer::Hist> _searchHist;
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
    void reloadFromComplete(sm::ReloadEvent) override;
    void reloadFromParsing(sm::ReloadEvent) override;
    void reloadFromIndexing(sm::ReloadEvent) override;

    inline void finish() {
        _sm.process_event(sm::FinishEvent{});
    }

    inline void paused() {
        _sm.process_event(sm::PausedEvent{});
    }

    inline void fail() {
        _sm.process_event(sm::FailEvent{});
    }

    void subscribeToSelectionChanged(LogTableModel* oldModel, LogTableModel* newModel);
    void applyFilter();
    void adaptFilter();

protected:
    virtual seer::task::Task* createIndexingTask(
        seer::Index* index,
        seer::FileParser* fileParser,
        seer::ILineParser* lineParser);

    virtual seer::task::Task* createParsingTask(seer::FileParser* fileParser);

public:
    LogFile(std::unique_ptr<std::istream>&& stream,
            std::shared_ptr<seer::ILineParser> lineParser);
    ~LogFile() = default;

    inline void parse() {
        _sm.process_event(sm::ParseEvent{});
    }

    inline void index() {
        _sm.process_event(sm::IndexEvent{});
    }

    inline void search(std::string text,
                       bool regex,
                       bool caseSensitive,
                       bool messageOnly) {
        _sm.process_event(
            sm::SearchEvent{text, regex, caseSensitive, messageOnly});
    }

    inline void interrupt() {
        _sm.process_event(sm::InterruptEvent{});
    }

    inline void reload(std::shared_ptr<std::istream> stream,
                       std::shared_ptr<seer::ILineParser> parser = {}) {
        _sm.process_event(sm::ReloadEvent{stream, parser});
    }

    inline std::string dbgStateName() {
        std::string name;
        _sm.visit_current_states([&](auto state) {
            name = state.c_str();
        });
        return name;
    }

    void requestFilter(int column);
    void setColumnFilter(int column, std::set<std::string> values);
    std::set<std::string> getColumnFilter(int column);
    LogTableModel* logTableModel();
    LogTableModel* searchLogTableModel();
    const seer::Hist* searchHist();
    seer::ILineParser* lineParser();
    void clearFilters();
    void clearFilter(int column);
    void excludeValue(int column, const std::string& value);
    void includeOnlyValue(int column, const std::string& value);

    template <class S>
    inline bool isState(S state) {
        return _sm.is(state);
    }

signals:
    void filterRequested(FilterTableModel* model, int column);
    void stateChanged();
    void progressChanged(int progress);
};

} // namespace gui
