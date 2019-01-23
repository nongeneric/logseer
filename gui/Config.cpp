#include "Config.h"

#include "seer/Log.h"
#include <QResource>
#include <boost/filesystem.hpp>
#include <fstream>
#include <regex>
#include <stdio.h>

using namespace boost::filesystem;

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
}

namespace gui {

    void Config::init() {
        auto dir = getConfigDirectory() / "regex";

        seer::log_infof("searching %s", dir.string().c_str());

        if (!exists(dir)) {
            seer::log_info("initializing the default set of regex configs");

            for (auto rname : { "200_journalctl.json" }) {
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
                seer::log_infof("parser config name '%s' doesn't have the right format", fileName.c_str());
                continue;
            }
            auto priority = std::stoi(match.str(1));
            auto name = match.str(2);
            auto json = read_all_text(p.path().string());
            _regexConfigs.push_back({name, priority, json});
        }
    }

    std::vector<RegexConfig> Config::regexConfigs() {
        return _regexConfigs;
    }

    Config g_Config;

} // namespace gui
