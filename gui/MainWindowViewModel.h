#pragma once

#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "LogTableModel.h"
#include <string>
#include <memory>

namespace gui {

    class LogFile {
        std::unique_ptr<seer::FileParser> _fileParser;
        std::unique_ptr<seer::ILineParser> _lineParser;
        std::unique_ptr<LogTableModel> _logTableModel;

    public:
        LogFile(std::string path);
        void parse();
        LogTableModel* logTableModel();
    };

    class MainWindowViewModel {
    public:
        MainWindowViewModel();
    };

} // namespace gui
