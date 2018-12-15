#pragma once

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include "seer/ParsingTask.h"
#include "seer/IndexingTask.h"
#include <QObject>
#include <memory>
#include <string>
#include <map>
#include <atomic>
#include <istream>

namespace gui {

    enum class LogFileState {
        Idle, Parsing, Parsed, Indexing, Complete
    };

    class LogFile : public QObject {
        Q_OBJECT
        std::unique_ptr<seer::FileParser> _fileParser;
        std::shared_ptr<seer::ILineParser> _lineParser;
        std::unique_ptr<seer::Index> _index;
        std::unique_ptr<seer::Index> _searchIndex;
        std::unique_ptr<LogTableModel> _logTableModel;
        std::unique_ptr<LogTableModel> _searchLogTableModel;
        std::map<int, std::vector<std::string>> _columnFilters;
        std::unique_ptr<std::istream> _stream;
        std::shared_ptr<seer::ILineParserRepository> _repository;
        std::atomic<LogFileState> _state = LogFileState::Idle;
        std::unique_ptr<seer::ParsingTask> _parsingTask;
        std::unique_ptr<seer::IndexingTask> _indexingTask;

    public:
        LogFile(std::unique_ptr<std::istream>&& stream,
                std::shared_ptr<seer::ILineParserRepository> repository);
        ~LogFile() = default;
        void parse();
        void stopParsing();
        void index();
        void stopIndexing();
        void requestFilter(int column);
        void setColumnFilter(int column, std::vector<std::string> values);
        LogTableModel* logTableModel();
        LogTableModel *searchLogTableModel(std::string text, bool caseSensitive);
        LogFileState state() const;

    signals:
        void filterRequested(FilterTableModel* model, int column);
        void parsingComplete();
        void parsingProgress(int progress);
        void indexingComplete();
        void indexingProgress(int progress);
    };

} // namespace gui
