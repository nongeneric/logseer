#pragma once

#include <vector>
#include <string>

namespace gui {

    struct RegexConfig {
        std::string name;
        int priority;
        std::string json;
    };

    class Config {
        std::vector<RegexConfig> _regexConfigs;

    public:
        void init();
        std::vector<RegexConfig> regexConfigs();
    };

    extern Config g_Config;

} // namespace gui
