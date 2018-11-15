#include <catch2/catch.hpp>

#include "seer/ILineParser.h"
#include "seer/FileParser.h"
#include "seer/Index.h"
#include <sstream>

using namespace seer;

std::string simpleLog = "10 INFO CORE message 1\n"
                        "15 INFO SUB message 2\n"
                        "17 WARN CORE message 3\n"
                        "20 INFO SUB message 4\n"
                        "30 ERR CORE message 5\n"
                        "40 WARN SUB message 6\n";

class TestLineParser : public ILineParser {
public:
    bool parseLine(std::string_view line,
                   std::vector<std::string>& columns) override {
        std::stringstream ss{std::string(line)};
        std::string c1, c2, c3, c4;
        std::getline(ss, c1, ' ');
        std::getline(ss, c2, ' ');
        std::getline(ss, c3, ' ');
        std::getline(ss, c4);
        columns = { c1, c2, c3, c4 };
        return true;
    }

    std::vector<ColumnFormat> getColumnFormats() override {
        return {{"Timestamp", false},
                {"Level", true},
                {"Component", true},
                {"Message", false}};
    }

    bool isMatch([[maybe_unused]] std::string_view sample,
                 [[maybe_unused]] std::string_view fileName) override {
        return true;
    }
};

TEST_CASE("simple_parser") {
    std::stringstream ss(simpleLog);
    TestLineParser lineParser;
    FileParser fileParser(&ss, &lineParser);
    fileParser.index([](auto, auto&){ return true; });

    REQUIRE(fileParser.lineCount() == 6);
    std::vector<std::string> line;
    fileParser.readLine(0, line);
    REQUIRE(line[0] == "10");
    REQUIRE(line[1] == "INFO");
    REQUIRE(line[2] == "CORE");
    REQUIRE(line[3] == "message 1");

    fileParser.readLine(2, line);
    REQUIRE(line[0] == "17");
    REQUIRE(line[1] == "WARN");
    REQUIRE(line[2] == "CORE");
    REQUIRE(line[3] == "message 3");

    fileParser.readLine(5, line);
    REQUIRE(line[0] == "40");
    REQUIRE(line[1] == "WARN");
    REQUIRE(line[2] == "SUB");
    REQUIRE(line[3] == "message 6");

    fileParser.readLine(1, line);
    REQUIRE(line[0] == "15");
    REQUIRE(line[1] == "INFO");
    REQUIRE(line[2] == "SUB");
    REQUIRE(line[3] == "message 2");
}

TEST_CASE("simple_index") {
    std::stringstream ss(simpleLog);
    TestLineParser lineParser;
    FileParser fileParser(&ss, &lineParser);

    Index index;
    index.index(&fileParser, &lineParser);
    REQUIRE( index.getLineCount() == 6 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(5) == 5 );

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 3 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 1 );
    REQUIRE( index.mapIndex(2) == 3 );

    filters = {{1, {"INFO", "ERR"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 4 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 1 );
    REQUIRE( index.mapIndex(2) == 3 );
    REQUIRE( index.mapIndex(3) == 4 );

    filters = {{1, {"INFO", "ERR"}}, {2, {"CORE"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 2 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 4 );

    filters = {{2, {"CORE"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 3 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 2 );
    REQUIRE( index.mapIndex(2) == 4 );
}

TEST_CASE("get_values") {
    std::stringstream ss(simpleLog);
    TestLineParser lineParser;
    FileParser fileParser(&ss, &lineParser);

    Index index;
    index.index(&fileParser, &lineParser);
    auto values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[0] == "ERR" );
    REQUIRE( values[1] == "INFO" );
    REQUIRE( values[2] == "WARN" );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0] == "SUB" );
    REQUIRE( values[1] == "CORE" );
}
