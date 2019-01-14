#pragma once

#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include "seer/RegexLineParser.h"
#include <sstream>

inline std::string simpleLog = "10 INFO CORE message 1\n"
                               "15 INFO SUB message 2\n"
                               "17 WARN CORE message 3\n"
                               "20 INFO SUB message 4\n"
                               "30 ERR CORE message 5\n"
                               "40 WARN SUB message 6\n";

inline std::string multilineLog = "10 INFO CORE message 1\n"
                                  "message 1 a\n"
                                  "message 1 b\n"
                                  "15 INFO SUB message 2\n"
                                  "17 WARN CORE message 3\n"
                                  "20 INFO SUB message 4\n"
                                  "30 ERR CORE message 5\n"
                                  "message 5 a\n"
                                  "message 5 b\n"
                                  "message 5 c\n"
                                  "40 WARN SUB message 6\n";

inline std::shared_ptr<seer::ILineParser> createTestParser() {
    auto config =
        R"_(
            {
                "description": "test description",
                "regex": "(\\d+) (.*?) (.*?) (.*)",
                "columns": [
                    {
                        "name": "Timestamp",
                        "group": 1,
                        "indexed": false
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
    auto parser = std::make_shared<seer::RegexLineParser>("test parser");
    parser->load(config);
    return parser;
}

class TestLineParserRepository : public seer::ILineParserRepository {
public:
    inline std::shared_ptr<seer::ILineParser> resolve(std::istream&) override {
        return createTestParser();
    }
};

inline constexpr int g_TestLogColumns = 4;
