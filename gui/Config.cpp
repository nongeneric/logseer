#include "Config.h"

#include "seer/Log.h"
#include <QResource>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdio.h>

using namespace boost::filesystem;
using namespace nlohmann;

namespace {
    std::vector<uint8_t> read_all_bytes(std::string_view path) {
        auto f = fopen(begin(path), "r");
        if (!f)
            throw std::runtime_error("can't open file");
        fseek(f, 0, SEEK_END);
        auto filesize = ftell(f);
        std::vector<uint8_t> res(filesize);
        fseek(f, 0, SEEK_SET);
        fread(&res[0], 1, res.size(), f);
        fclose(f);
        return res;
    }

    std::string read_all_text(std::string_view path) {
        auto vec = read_all_bytes(path);
        return std::string((const char*)&vec[0], vec.size());
    }

    void write_all_bytes(const void* ptr, uint32_t size, std::string_view path) {
        auto f = fopen(begin(path), "w");
        if (!f)
            throw std::runtime_error("can't open file");
        auto res = fwrite(ptr, 1, size, f);
        if (res != size)
            throw std::runtime_error("incomplete write");
        fclose(f);
    }

    path getHomeDirectory() {
        auto userProfile = std::getenv("USERPROFILE");
        if (userProfile)
            return userProfile;
        auto homeDrive = std::getenv("HOMEDRIVE");
        auto homePath = std::getenv("HOMEPATH");
        if (homeDrive && homePath)
            return path(homeDrive) / homePath;
        auto home = std::getenv("HOME");
        if (home)
            return home;
        return {};
    }

    path getConfigDirectory() {
        auto home = getHomeDirectory();
        if (home.empty()) {
            home = boost::filesystem::current_path();
        }
        return home / ".logseer";
    }

    path getConfigJsonPath() {
        return getConfigDirectory() / "logseer.json";
    }
}

namespace gui {

    void Config::initRegexConfigs() {
        auto dir = getConfigDirectory() / "regex";

        seer::log_infof("searching directory [%s]", dir.string());

        if (!exists(dir)) {
            seer::log_info("initializing the default set of regex configs");

            for (auto rname : { "200_journalctl.json", "500_logseer.json" }) {
                create_directories(dir);
                QResource resource(QString(":/") + rname);
                write_all_bytes(resource.data(), resource.size(), (dir / rname).string());
            }
        }

        std::regex rxFileName{R"(^(\d\d\d)_(.*?)\.json$)"};

        for (auto& p : directory_iterator(dir)) {
            if (!is_regular_file(p))
                continue;
            std::smatch match;
            auto fileName = p.path().filename().string();
            if (!std::regex_search(fileName, match, rxFileName)) {
                seer::log_infof("parser config name [%s] doesn't have the right format", fileName);
                continue;
            }
            auto priority = std::stoi(match.str(1));
            auto name = match.str(2);
            auto json = read_all_text(p.path().string());
            _regexConfigs.push_back({name, priority, json});
        }
    }

    void Config::save() {
        json j = {
            {
                "font", {
                    {"name", _fontConfig.name},
                    {"size", _fontConfig.size}
                }
            },
            {
                "search", {
                    {"messageOnly", _searchConfig.messageOnly},
                    {"regex", _searchConfig.regex},
                    {"caseSensitive", _searchConfig.caseSensitive}
                }
            }
        };
        auto str = j.dump(4);
        write_all_bytes(&str[0], str.size(), getConfigJsonPath().string());
    }

    void Config::init() {
        initRegexConfigs();

        auto configPath = getConfigJsonPath();
        if (!is_regular_file(configPath)) {
            seer::log_infof("no config found at [%s]", configPath);
            save();
            return;
        }

        std::stringstream ss(read_all_text(configPath.string()));
        json j;
        ss >> j;
        auto font = j["font"];
        _fontConfig.name = font["name"].get<std::string>();
        _fontConfig.size = font["size"].get<int>();
        auto search = j["search"];
        _searchConfig.messageOnly = search["messageOnly"].get<bool>();
        _searchConfig.regex = search["regex"].get<bool>();
        _searchConfig.caseSensitive = search["caseSensitive"].get<bool>();
    }

    std::vector<RegexConfig> Config::regexConfigs() {
        return _regexConfigs;
    }

    FontConfig Config::fontConfig() {
        return _fontConfig;
    }

    SearchConfig Config::searchConfig() {
        return _searchConfig;
    }

    void Config::save(FontConfig const& config) {
        _fontConfig = config;
        save();
    }

    void Config::save(SearchConfig const& config) {
        _searchConfig = config;
        save();
    }

    Config g_Config;

} // namespace gui
