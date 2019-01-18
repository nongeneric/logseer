#include "CommandLineParser.h"
#include <boost/program_options.hpp>
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
                _version = ssnprintf("logseer %s", g_version);
                return true;
            }

            po::notify(console_vm);
        } catch(std::exception& e) {
            _error = ssnprintf("can't parse program options:\n%s\n\n%s",
                               e.what(),
                               help().c_str());
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

} // namespace seer
