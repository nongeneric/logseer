#include "Config.h"

#include "seer/Log.h"
#include <QResource>
#include <nlohmann/json.hpp>
#include <fstream>
#include <regex>
#include <sstream>
#include <fstream>

using namespace boost::filesystem;
using namespace nlohmann;

namespace {

struct {
    const char* fontGroup = "font";
    const char* fontName = "name";
    const char* fontSize = "size";
    const char* searchGroup = "search";
    const char* searchMessageOnly = "messageOnly";
    const char* searchRegex = "regex";
    const char* searchCaseSensitive = "caseSensitive";
    const char* sessionGroup = "session";
    const char* sessionOpenedFiles = "openedFiles";
    const char* sessionOpenedFilePath = "path";
    const char* sessionOpenedFileParser = "parser";
    const char* sessionRecentFiles = "recentFiles";
    const char* generalGroup = "general";
    const char* showCloseTabButton = "showCloseTabButton";
    const char* maxThreads = "maxThreads";
} g_consts;

} // namespace

namespace gui {

path Config::getConfigDirectory() {
    auto home = getHomeDirectory();
    if (home.empty()) {
        home = boost::filesystem::current_path();
    }
    return home / ".logseer";
}

path Config::getHomeDirectory() {
    auto userProfile = _fileSystem->getenv("USERPROFILE");
    if (!userProfile.empty())
        return userProfile;
    auto homeDrive = _fileSystem->getenv("HOMEDRIVE");
    auto homePath = _fileSystem->getenv("HOMEPATH");
    if (!homeDrive.empty() && !homePath.empty())
        return path(homeDrive) / homePath;
    auto home = _fileSystem->getenv("HOME");
    if (!home.empty())
        return home;
    return {};
}

path Config::getConfigJsonPath() {
    return getConfigDirectory() / "logseer.json";
}

void Config::initRegexConfigs() {
    auto dir = getConfigDirectory() / "regex";

    seer::log_infof("searching directory [{}]", dir.string());

    if (!exists(dir)) {
        seer::log_info("initializing the default set of regex configs");

        for (auto rname : { "200_journalctl.json", "500_logseer.json" }) {
            QResource resource(QString(":/") + rname);
            _fileSystem->writeFile(dir / rname, resource.data(), resource.size());
        }
    }

    std::regex rxFileName{R"(^(\d\d\d)_(.*?)\.json$)"};

    for (auto& p : _fileSystem->files(dir)) {
        std::smatch match;
        auto fileName = p.filename().string();
        if (!std::regex_search(fileName, match, rxFileName)) {
            seer::log_infof("parser config name [{}] doesn't have the right format", fileName);
            continue;
        }
        auto priority = std::stoi(match.str(1));
        auto name = match.str(2);
        auto json = _fileSystem->readFile(p);
        _regexConfigs.push_back({name, priority, json});
    }
}

void Config::save() {
    _sessionConfig.recentFiles.resize(
        std::min<size_t>(g_maxRecentFiles, _sessionConfig.recentFiles.size()));

    json j = {
        {
            g_consts.fontGroup, {
                {g_consts.fontName, _fontConfig.name},
                {g_consts.fontSize, _fontConfig.size}
            }
        },
        {
            g_consts.searchGroup, {
                {g_consts.searchMessageOnly, _searchConfig.messageOnly},
                {g_consts.searchRegex, _searchConfig.regex},
                {g_consts.searchCaseSensitive, _searchConfig.caseSensitive}
            }
        },
        {
            g_consts.sessionGroup, {
                {g_consts.sessionOpenedFiles, 0},
                {g_consts.sessionRecentFiles, _sessionConfig.recentFiles},
            }
        },
        {
            g_consts.generalGroup, {
                {g_consts.showCloseTabButton, _generalConfig.showCloseTabButton},
                {g_consts.maxThreads, _generalConfig.maxThreads},
            }
        }
    };

    auto openedFiles = json::array();
    for (auto& [path, parser] : _sessionConfig.openedFiles) {
        auto file = json{
            {g_consts.sessionOpenedFilePath, path},
            {g_consts.sessionOpenedFileParser, parser}
        };
        openedFiles.push_back(file);
    }

    j[g_consts.sessionGroup][g_consts.sessionOpenedFiles] = openedFiles;

    auto str = j.dump(4);
    _fileSystem->writeFile(getConfigJsonPath(), &str[0], str.size());
}

void Config::init(std::shared_ptr<IFileSystem> fileSystem) {
    _fileSystem = fileSystem;
    initRegexConfigs();

    auto configPath = getConfigJsonPath();
    auto files = _fileSystem->files(configPath.parent_path());
    if (std::find(begin(files), end(files), configPath) == end(files)) {
        seer::log_infof("no config found at [{}]", configPath.string());
        save();
        return;
    }

    std::stringstream ss(_fileSystem->readFile(configPath));
    json j;
    ss >> j;
    auto font = j[g_consts.fontGroup];
    _fontConfig.name = font[g_consts.fontName].get<std::string>();
    _fontConfig.size = font[g_consts.fontSize].get<int>();
    auto search = j[g_consts.searchGroup];
    _searchConfig.messageOnly = search[g_consts.searchMessageOnly].get<bool>();
    _searchConfig.regex = search[g_consts.searchRegex].get<bool>();
    _searchConfig.caseSensitive = search[g_consts.searchCaseSensitive].get<bool>();

    auto general = j[g_consts.generalGroup];
    auto jShowCloseTabButton = general[g_consts.showCloseTabButton];
    if (!jShowCloseTabButton.is_null()) {
        _generalConfig.showCloseTabButton = jShowCloseTabButton.get<bool>();
    }

    auto jMaxThreads = general[g_consts.maxThreads];
    if (!jMaxThreads.is_null()) {
        _generalConfig.maxThreads = jMaxThreads.get<unsigned>();
    }

    auto session = j[g_consts.sessionGroup];
    if (!session.is_null()) {
        auto openedFiles = session[g_consts.sessionOpenedFiles];
        if (openedFiles.is_array()) {
            for (auto openedFile : openedFiles) {
                auto path = openedFile[g_consts.sessionOpenedFilePath].get<std::string>();
                auto parser = openedFile[g_consts.sessionOpenedFileParser].get<std::string>();
                _sessionConfig.openedFiles.push_back({path, parser});
            }
        }
        auto recentFiles = session[g_consts.sessionRecentFiles];
        if (recentFiles.is_array()) {
            _sessionConfig.recentFiles = recentFiles.get<std::vector<std::string>>();
        }
    }
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

SessionConfig Config::sessionConfig() {
    return _sessionConfig;
}

GeneralConfig Config::generalConfig() {
    return _generalConfig;
}

void Config::save(FontConfig const& config) {
    _fontConfig = config;
    save();
}

void Config::save(SearchConfig const& config) {
    _searchConfig = config;
    save();
}

void Config::save(const SessionConfig &config) {
    _sessionConfig = config;
    save();
}

Config g_Config;

std::string RuntimeFileSystem::readFile(path path) {
    std::ifstream f(path.string());
    if (!f.is_open())
        throw std::runtime_error("can't open file");
    f.seekg(0, std::ios_base::end);
    auto filesize = f.tellg();
    std::string res(filesize, '\0');
    f.seekg(0);
    f.read(&res[0], res.size());
    return res;
}

void RuntimeFileSystem::writeFile(path path, const void* content, unsigned size) {
    create_directories(path.parent_path());
    std::ofstream f(path.string());
    if (!f.is_open())
        throw std::runtime_error("can't open file");
    f.write(reinterpret_cast<const char*>(content), size);
}

std::vector<path> RuntimeFileSystem::files(path directory) {
    std::vector<path> res;
    for (auto& p : directory_iterator(directory)) {
        if (!is_regular_file(p))
            continue;
        res.push_back(p.path());
    }
    return res;
}

std::string RuntimeFileSystem::getenv(const char* name) {
    auto value = std::getenv(name);
    return value ? value : "";
}

} // namespace gui
