#pragma once

#include <string>
#include <vector>

namespace seer {

class CommandLineParser {
    std::string _help;
    std::string _error;
    std::string _version;
    std::vector<std::string> _paths;
    bool _verbose = false;

public:
    bool parse(int argc, const char* const argv[]);
    std::string help() const;
    std::string error() const;
    std::string version() const;
    std::vector<std::string> paths() const;
    bool verbose() const;
};

} // namespace seer
