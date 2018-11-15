#pragma once

#include "LogTableModel.h"
#include "FilterTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include <QObject>
#include <memory>
#include <string>
#include <map>

namespace gui {

    class LogFile : public QObject {
        Q_OBJECT
        std::unique_ptr<seer::FileParser> _fileParser;
        std::unique_ptr<seer::ILineParser> _lineParser;
        std::unique_ptr<seer::Index> _index;
        std::unique_ptr<LogTableModel> _logTableModel;
        std::map<int, std::vector<std::string>> _columnFilters;
        std::string _path;

    public:
        LogFile(std::string path);
        void parse();
        void requestFilter(int column);
        void setColumnFilter(int column, std::vector<std::string> values);
        LogTableModel* logTableModel();

    signals:
        void filterRequested(FilterTableModel* model, int column);
    };

} // namespace gui
