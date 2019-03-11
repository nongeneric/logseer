#pragma once

#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include "seer/LineParserRepository.h"
#include "seer/RegexLineParser.h"
#include <sstream>

inline std::string unstructuredLog = "message1\n"
                                     "message2\n"
                                     "message3\n"
                                     "message4\n";

inline std::string zeroesLog = std::string("message1\0\0\0\0\0\0\0\0\0\0", 18);

inline std::string simpleLog = "10 INFO CORE message 1\n"
                               "15 INFO SUB message 2\n"
                               "17 WARN CORE message 3\n"
                               "20 INFO SUB message 4\n"
                               "30 ERR CORE message 5\n"
                               "40 WARN SUB message 6\n";

inline std::string unicodeLog = u8"10 ИНФО CORE message 1\n"
                                u8"15 ИНФО SUB message 2\n"
                                u8"17 WARN CORE message 3\n"
                                u8"20 ИНФО SUB GRÜẞEN 4\n"
                                u8"30 ERR CORE GRÜSSEN 5\n"
                                u8"40 WARN SUB grüßen 6\n";

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

inline std::string threeColumnLog = "10 INFO CORE F1 message 1\n"
                                    "15 INFO SUB F2 message 2\n"
                                    "17 WARN CORE F1 message 3\n"
                                    "20 INFO SUB F2 message 4\n"
                                    "30 ERR CORE F1 message 5\n"
                                    "40 WARN SUB F2 message 6\n";

inline std::string testConfig =
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
            ],
            "colors": [
                {
                    "column": "Level",
                    "value": "ERR",
                    "color": "ff0000"
                },
                {
                    "column": "Level",
                    "value": "WARN",
                    "color": "550000"
                }
            ]
        }
    )_";

inline std::string threeColumnTestConfig =
    R"_(
        {
            "description": "test description",
            "regex": "(\\d+) (.*?) (.*?) (.*?) (.*)",
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
                    "name": "Flag",
                    "group": 4,
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

inline std::shared_ptr<seer::ILineParser> createTestParser(std::istream& stream) {
    seer::LineParserRepository repository;
    repository.addRegexParser("test parser", 0, testConfig);
    return repository.resolve(stream);
}

inline std::shared_ptr<seer::ILineParser> createThreeColumnTestParser(std::istream& stream) {
    seer::LineParserRepository repository;
    repository.addRegexParser("test parser", 0, threeColumnTestConfig);
    return repository.resolve(stream);
}

class TestLineParserRepository : public seer::ILineParserRepository {
public:
    inline std::shared_ptr<seer::ILineParser> resolve(std::istream& stream) override {
        return createTestParser(stream);
    }
};

inline constexpr int g_TestLogColumns = 4;
