#pragma once

#include <vector>
#include <string>
#include <stdint.h>

namespace gui {

    struct RegexConfig {
        std::string name;
        int priority;
        std::string json;
    };

    struct FontConfig {
        std::string name = "Mono";
        int size = 10;
    };

    struct SearchConfig {
        bool caseSensitive = false;
        bool regex = false;
        bool messageOnly = false;
    };

    class Config {
        std::vector<RegexConfig> _regexConfigs;
        FontConfig _fontConfig;
        SearchConfig _searchConfig;

        void initRegexConfigs();
        void save();

    public:
        void init();
        std::vector<RegexConfig> regexConfigs();
        FontConfig fontConfig();
        SearchConfig searchConfig();
        void save(FontConfig const& config);
        void save(SearchConfig const& config);
    };

    extern Config g_Config;

} // namespace gui
