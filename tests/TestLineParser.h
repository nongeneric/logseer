#pragma once

#include "seer/ILineParser.h"
#include "seer/ILineParserRepository.h"
#include <sstream>

inline std::string simpleLog = "10 INFO CORE message 1\n"
                               "15 INFO SUB message 2\n"
                               "17 WARN CORE message 3\n"
                               "20 INFO SUB message 4\n"
                               "30 ERR CORE message 5\n"
                               "40 WARN SUB message 6\n";

class TestLineParser : public seer::ILineParser {
public:
    inline bool parseLine(std::string_view line,
                          std::vector<std::string>& columns) override {
        std::stringstream ss{std::string(line)};
        std::string c1, c2, c3, c4;
        std::getline(ss, c1, ' ');
        std::getline(ss, c2, ' ');
        std::getline(ss, c3, ' ');
        std::getline(ss, c4);
        columns = {c1, c2, c3, c4};
        return true;
    }

    inline std::vector<seer::ColumnFormat> getColumnFormats() override {
        return {{"Timestamp", false},
                {"Level", true},
                {"Component", true},
                {"Message", false}};
    }

    inline bool isMatch([[maybe_unused]] std::string_view sample,
                        [[maybe_unused]] std::string_view fileName) override {
        return true;
    }
};

class TestLineParserRepository : public seer::ILineParserRepository {
public:
    inline std::shared_ptr<seer::ILineParser> resolve(std::istream&) override {
        return std::make_shared<TestLineParser>();
    }
};
