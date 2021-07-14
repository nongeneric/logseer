#include <catch2/catch.hpp>

#include "gui/Config.h"
#include "seer/bformat.h"
#include "seer/RegexLineParser.h"
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
                    {
                        "path": "/file/a",
                        "parser": "parser A"
                    }
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
    REQUIRE( session.openedFiles[0].path == "/file/a" );
    REQUIRE( session.openedFiles[0].parser == "parser A" );

    session.openedFiles.push_back({"/file/b", "parser B"});
    config.save(session);

    Config config2;
    config2.init(fs);

    session = config2.sessionConfig();
    REQUIRE( session.openedFiles.size() == 2 );
    REQUIRE( session.openedFiles[0].path == "/file/a" );
    REQUIRE( session.openedFiles[0].parser == "parser A" );
    REQUIRE( session.openedFiles[1].path == "/file/b" );
    REQUIRE( session.openedFiles[1].parser == "parser B" );
}

TEST_CASE("config_parse_recent_files") {
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
                "recentFiles": [
                    "/file/a",
                    "/file/b"
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
    REQUIRE( session.recentFiles.size() == 2 );
    REQUIRE( session.recentFiles[0] == "/file/a" );
    REQUIRE( session.recentFiles[1] == "/file/b" );

    session.recentFiles.push_back("/file/c");
    config.save(session);

    Config config2;
    config2.init(fs);

    session = config2.sessionConfig();
    REQUIRE( session.recentFiles.size() == 3 );
    REQUIRE( session.recentFiles[0] == "/file/a" );
    REQUIRE( session.recentFiles[1] == "/file/b" );
    REQUIRE( session.recentFiles[2] == "/file/c" );

    session.recentFiles.clear();
    for (auto i = 0; i < 2 * g_maxRecentFiles; ++i) {
        session.recentFiles.push_back(bformat("file %d.txt", i));
    }
    config2.save(session);

    config2.init(fs);
    session = config2.sessionConfig();
    REQUIRE( session.recentFiles.size() == g_maxRecentFiles );
    REQUIRE( session.recentFiles[0] == "file 0.txt" );
}

TEST_CASE("report_regex_parser_error") {
    auto json = "";
    REQUIRE_THROWS_AS(seer::RegexLineParser("").load(json), seer::JsonParserException);

    json =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) )()((.*?) (.*?) (.*)",
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false,
                        "autosize": true
                    },
                    {
                        "name": "Level",
                        "group": 2,
                        "indexed": true
                    },
                    {
                        "name": "Component",
                        "group": 3,
                        "indexed": true
                    },
                    {
                        "name": "Message",
                        "group": 4,
                        "indexed": false
                    }
                ]
            }
        )_";
    REQUIRE_THROWS_AS(seer::RegexLineParser("").load(json), seer::RegexpSyntaxException);

    json =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) (.*?) (.*?) (.*)",
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false,
                        "autosize": true
                    },
                    {
                        "name": "Level",
                        "group": 2,
                        "indexed": true
                    },
                    {
                        "name": "Component",
                        "group": 3,
                        "indexed": true
                    },
                    {
                        "name": "Message",
                        "group": 5,
                        "indexed": false
                    }
                ]
            }
        )_";
    REQUIRE_THROWS_AS(seer::RegexLineParser("").load(json), seer::RegexpOutOfBoundGroupReferenceException);
}

TEST_CASE("match_using_magic") {
    auto json =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) (.*?) (.*?) (.*)",
                "magic": "123",
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false,
                        "autosize": true
                    },
                    {
                        "name": "Level",
                        "group": 2,
                        "indexed": true
                    },
                    {
                        "name": "Component",
                        "group": 3,
                        "indexed": true
                    },
                    {
                        "name": "Message",
                        "group": 4,
                        "indexed": false
                    }
                ]
            }
        )_";

    seer::RegexLineParser parser{""};
    parser.load(json);

    REQUIRE( !parser.isMatch({""}, "") );
    REQUIRE( !parser.isMatch({"abcde"}, "") );
    REQUIRE( parser.isMatch({"12345"}, "") );
}

TEST_CASE("forbid_both_lua_and_magic") {
    auto json =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) (.*?) (.*?) (.*)",
                "magic": "123",
                "detector": [ "" ],
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false,
                        "autosize": true
                    },
                    {
                        "name": "Level",
                        "group": 2,
                        "indexed": true
                    },
                    {
                        "name": "Component",
                        "group": 3,
                        "indexed": true
                    },
                    {
                        "name": "Message",
                        "group": 4,
                        "indexed": false
                    }
                ]
            }
        )_";

    REQUIRE_THROWS_AS(seer::RegexLineParser("").load(json), seer::OptionInconsistencyException);
}

TEST_CASE("lua_detect_by_filename") {
    auto json =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) (.*?) (.*?) (.*)",
                "detector": [
                    "return string.find(fileName, '.ext') ~= nil"
                ],
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false,
                        "autosize": true
                    },
                    {
                        "name": "Level",
                        "group": 2,
                        "indexed": true
                    },
                    {
                        "name": "Component",
                        "group": 3,
                        "indexed": true
                    },
                    {
                        "name": "Message",
                        "group": 4,
                        "indexed": false
                    }
                ]
            }
        )_";

    seer::RegexLineParser parser{""};
    parser.load(json);

    REQUIRE( !parser.isMatch({""}, "file") );
    REQUIRE( parser.isMatch({""}, "file.ext") );
}

TEST_CASE("lua_detect_by_parsed_status") {
    auto json =
        R"_(
            {
                "description": "test description",
                "regex": "(A|B)",
                "detector": [
                    "return #lines >= 2 and lines[0].parsed and lines[1].parsed"
                ],
                "columns": [
                    {
                        "name": "message",
                        "group": 1,
                        "indexed": false
                    }
                ]
            }
        )_";

    seer::RegexLineParser parser{""};
    parser.load(json);

    REQUIRE( parser.isMatch({"A", "B", "A"}, "") );
    REQUIRE( !parser.isMatch({"A", "C", "A", "B"}, "") );
}

TEST_CASE("lua_detect_by_content") {
    auto json =
        R"_(
            {
                "description": "test description",
                "regex": "(A|B)",
                "detector": [
                    "return #lines >= 2 and lines[0].text == 'A'"
                ],
                "columns": [
                    {
                        "name": "message",
                        "group": 1,
                        "indexed": false
                    }
                ]
            }
        )_";

    seer::RegexLineParser parser{""};
    parser.load(json);

    REQUIRE( parser.isMatch({"A", "B", "A"}, "") );
    REQUIRE( !parser.isMatch({"B", "B", "A"}, "") );
}