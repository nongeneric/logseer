#include <catch2/catch.hpp>

#include "gui/Config.h"
#include <map>

using namespace boost::filesystem;
using namespace gui;

class TestFileSystem : public gui::IFileSystem {
public:
    std::map<path, std::string> fileMap;
    std::map<std::string, std::string> environment;

    std::string readFile(path path) override {
        auto file = fileMap.find(path);
        if (file == end(fileMap))
            throw std::runtime_error("not found");
        return file->second;
    }

    void writeFile(path path, const void* content, unsigned size) override {
        fileMap[path] = std::string(reinterpret_cast<const char*>(content), size);
    }

    std::vector<path> files(path) override {
        std::vector<path> res;
        for (auto& [name, _] : fileMap) {
            res.push_back(name);
        }
        return res;
    }

    std::string getenv(const char* name) override {
        auto it = environment.find(name);
        if (it == end(environment))
            return {};
        return it->second;
    }
};

TEST_CASE("config_initialize_new") {
    auto fs = std::make_shared<TestFileSystem>();
    fs->environment["HOME"] = "/home/user";
    Config config;
    config.init(fs);
    REQUIRE( fs->fileMap.size() > 0 );
    auto primary = fs->fileMap.find("/home/user/.logseer/logseer.json");
    REQUIRE( primary != end(fs->fileMap) );
}

TEST_CASE("config_parse_default") {
    auto fs = std::make_shared<TestFileSystem>();
    fs->environment["HOME"] = "/home/user";
    Config config;
    config.init(fs);
    config.init(fs);
}

TEST_CASE("config_parse_existing") {
    auto json = R"(
        {
            "font": {
                "name": "Liberation Mono",
                "size": 12
            },
            "search": {
                "caseSensitive": true,
                "messageOnly": false,
                "regex": true
            }
        }
    )";

    auto fs = std::make_shared<TestFileSystem>();
    fs->environment["HOME"] = "/home/user";
    fs->fileMap["/home/user/.logseer/logseer.json"] = json;
    Config config;
    config.init(fs);

    REQUIRE( config.fontConfig().name == "Liberation Mono" );
    REQUIRE( config.fontConfig().size == 12 );
    REQUIRE( config.searchConfig().caseSensitive == true );
    REQUIRE( config.searchConfig().messageOnly == false );
    REQUIRE( config.searchConfig().regex == true );
}

TEST_CASE("config_parse_opened_files") {
    auto json = R"(
        {
            "font": {
                "name": "Liberation Mono",
                "size": 12
            },
            "search": {
                "caseSensitive": true,
                "messageOnly": false,
                "regex": true
            },
            "session": {
                "openedFiles": [
                    "/file/a"
                ]
            }
        }
    )";

    auto fs = std::make_shared<TestFileSystem>();
    fs->environment["HOME"] = "/home/user";
    fs->fileMap["/home/user/.logseer/logseer.json"] = json;
    Config config;
    config.init(fs);

    auto session = config.sessionConfig();
    REQUIRE( session.openedFiles.size() == 1 );
    REQUIRE( session.openedFiles[0] == "/file/a" );

    session.openedFiles.push_back("/file/b");
    config.save(session);

    Config config2;
    config2.init(fs);

    session = config.sessionConfig();
    REQUIRE( session.openedFiles.size() == 2 );
    REQUIRE( session.openedFiles[0] == "/file/a" );
    REQUIRE( session.openedFiles[1] == "/file/b" );
}
