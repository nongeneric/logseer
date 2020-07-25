#pragma once

#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <stdint.h>

namespace gui {

inline constexpr int g_maxRecentFiles = 15;
inline constexpr const char* g_socketName = "logseer.socket";
inline constexpr const char* g_qtStyleName = "fusion";

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

struct OpenedFile {
    std::string path;
    std::string parser;
};

struct SessionConfig {
    std::vector<OpenedFile> openedFiles;
    std::vector<std::string> recentFiles;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual std::string readFile(boost::filesystem::path path) = 0;
    virtual void writeFile(boost::filesystem::path path, const void* content, unsigned size) = 0;
    virtual std::vector<boost::filesystem::path> files(boost::filesystem::path directory) = 0;
    virtual std::string getenv(const char* name) = 0;
};

class RuntimeFileSystem : public IFileSystem {
public:
    std::string readFile(boost::filesystem::path path) override;
    void writeFile(boost::filesystem::path path, const void* content, unsigned size) override;
    std::vector<boost::filesystem::path> files(boost::filesystem::path directory) override;
    std::string getenv(const char* name) override;
};

class Config {
    std::vector<RegexConfig> _regexConfigs;
    FontConfig _fontConfig;
    SearchConfig _searchConfig;
    SessionConfig _sessionConfig;
    std::shared_ptr<IFileSystem> _fileSystem;

    boost::filesystem::path getHomeDirectory();
    boost::filesystem::path getConfigJsonPath();
    void initRegexConfigs();
    void save();

public:
    void init(std::shared_ptr<IFileSystem> fileSystem = std::make_shared<RuntimeFileSystem>());
    std::vector<RegexConfig> regexConfigs();
    FontConfig fontConfig();
    SearchConfig searchConfig();
    SessionConfig sessionConfig();
    void save(FontConfig const& config);
    void save(SearchConfig const& config);
    void save(SessionConfig const& config);
    boost::filesystem::path getConfigDirectory();
};

extern Config g_Config;

} // namespace gui
