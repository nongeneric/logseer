#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <stdint.h>
#include <memory>

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
    std::string name =
#ifdef WIN32
        "Courier";
#else
        "Mono";
#endif
    int size = 10;
};

struct SearchConfig {
    bool caseSensitive = false;
    bool regex = false;
    bool unicodeAware = false;
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

struct GeneralConfig {
    bool showCloseTabButton = true;
    unsigned maxThreads = 10;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual std::string readFile(std::filesystem::path path) = 0;
    virtual void writeFile(std::filesystem::path path, const void* content, unsigned size) = 0;
    virtual std::vector<std::filesystem::path> files(std::filesystem::path directory) = 0;
    virtual std::string getenv(const char* name) = 0;
};

class RuntimeFileSystem : public IFileSystem {
public:
    std::string readFile(std::filesystem::path path) override;
    void writeFile(std::filesystem::path path, const void* content, unsigned size) override;
    std::vector<std::filesystem::path> files(std::filesystem::path directory) override;
    std::string getenv(const char* name) override;
};

class Config {
    std::vector<RegexConfig> _regexConfigs;
    FontConfig _fontConfig;
    SearchConfig _searchConfig;
    SessionConfig _sessionConfig;
    GeneralConfig _generalConfig;
    std::shared_ptr<IFileSystem> _fileSystem;

    std::filesystem::path getHomeDirectory();
    std::filesystem::path getConfigJsonPath();
    void initRegexConfigs();
    void save();

public:
    void init(std::shared_ptr<IFileSystem> fileSystem = std::make_shared<RuntimeFileSystem>());
    std::vector<RegexConfig> regexConfigs();
    FontConfig fontConfig();
    SearchConfig searchConfig();
    SessionConfig sessionConfig();
    GeneralConfig generalConfig();
    void save(FontConfig const& config);
    void save(SearchConfig const& config);
    void save(SessionConfig const& config);
    std::filesystem::path getConfigDirectory();
};

extern Config g_Config;

} // namespace gui
