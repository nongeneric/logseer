#pragma once

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include <QObject>
#include <memory>
#include <string>
#include <map>
#include <istream>

namespace gui {

    class LogFile : public QObject {
        Q_OBJECT
        std::unique_ptr<seer::FileParser> _fileParser;
        std::shared_ptr<seer::ILineParser> _lineParser;
        std::unique_ptr<seer::Index> _index;
        std::unique_ptr<seer::Index> _searchIndex;
        std::unique_ptr<LogTableModel> _logTableModel;
        std::unique_ptr<LogTableModel> _searchLogTableModel;
        std::map<int, std::vector<std::string>> _columnFilters;
        bool _parsed = false;
        std::unique_ptr<std::istream> _stream;
        std::shared_ptr<seer::ILineParserRepository> _repository;

    public:
        LogFile(std::unique_ptr<std::istream>&& stream,
                std::shared_ptr<seer::ILineParserRepository> repository);
        ~LogFile() = default;
        void parse();
        void requestFilter(int column);
        void setColumnFilter(int column, std::vector<std::string> values);
        LogTableModel* logTableModel();
        LogTableModel *searchLogTableModel(std::string text);

    signals:
        void filterRequested(FilterTableModel* model, int column);
    };

} // namespace gui
