#include "CommandLineParser.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "seer/Log.h"
#include "version.h"
#include <sstream>

namespace po = boost::program_options;

namespace seer {

bool CommandLineParser::parse(int argc, const char* const argv[]) {
    po::options_description console_desc("Allowed options");
    try {
        auto logName = "log";

        po::positional_options_description pd;
        pd.add(logName, -1);

        console_desc.add_options()
            ("help", "produce help message")
            (logName, po::value(&_paths), "log file path")
            ("version,v", "print version")
            ("verbose", "enable logging")
            ("file-logging", "log to file in addition to console")
            ;

        po::variables_map console_vm;

        auto parsed = po::command_line_parser(argc, argv)
                    .options(console_desc)
                    .positional(pd)
                    .run();

        po::store(parsed, console_vm);

        auto help = [&] {
            std::stringstream ss;
            ss << console_desc;
            return ss.str();
        };

        if (console_vm.count("help")) {
            _help = help();
            return true;
        }

        if (console_vm.count("version")) {
            _version = fmt::format("logseer {}", g_version);
            return true;
        }

        _verbose = console_vm.count("verbose");
        _fileLog = console_vm.count("file-logging");

        po::notify(console_vm);

        for (auto& path : _paths) {
            path = boost::filesystem::absolute(path).string();
        }
    } catch(std::exception& e) {
        _error = fmt::format(
            "can't parse program options:\n{}\n\n{}", e.what(), help());
        return false;
    }

    return true;
}

std::string CommandLineParser::help() const {
    return _help;
}

std::string CommandLineParser::error() const {
    return _error;
}

std::string CommandLineParser::version() const {
    return _version;
}

std::vector<std::string> CommandLineParser::paths() const {
    return _paths;
}

bool CommandLineParser::verbose() const {
    return _verbose;
}

bool CommandLineParser::fileLog() const {
    return _fileLog;
}

} // namespace seer
