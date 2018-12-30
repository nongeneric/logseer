#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "seer/ILineParser.h"
#include "seer/FileParser.h"
#include "seer/Index.h"
#include <sstream>

using namespace seer;

TEST_CASE("simple_parser") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

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
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
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
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    auto values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[0].value == "ERR" );
    REQUIRE( values[1].value == "INFO" );
    REQUIRE( values[2].value == "WARN" );
    REQUIRE( values[0].count == 1 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[2].count == 2 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );
    REQUIRE( values[2].checked == true );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0].value == "SUB" );
    REQUIRE( values[1].value == "CORE" );
    REQUIRE( values[0].count == 3 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    index.filter(filters);
    values = index.getValues(1);
    REQUIRE( values[0].checked == false );
    REQUIRE( values[1].checked == true );
    REQUIRE( values[2].checked == false );
}

TEST_CASE("search") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    REQUIRE( index.getLineCount() == 6 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(5) == 5 );

    auto indexCopy = index;
    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    indexCopy.filter(filters);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 3 );

    indexCopy.search(&fileParser, "4", true);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );

    indexCopy = index;
    indexCopy.search(&fileParser, "4", true);
    REQUIRE( indexCopy.getLineCount() == 2 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );
    REQUIRE( indexCopy.mapIndex(1) == 5 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", true);
    REQUIRE( indexCopy.getLineCount() == 0 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", false);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 3 );
}

TEST_CASE("multiline_index") {
    std::stringstream ss(multilineLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    REQUIRE( index.getLineCount() == 11 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(5) == 5 );
    REQUIRE( index.mapIndex(10) == 10 );

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 5 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 1 );
    REQUIRE( index.mapIndex(2) == 2 );
    REQUIRE( index.mapIndex(3) == 3 );
    REQUIRE( index.mapIndex(4) == 5 );

    filters = {{1, {"INFO", "ERR"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 9 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(1) == 1 );
    REQUIRE( index.mapIndex(4) == 5 );
    REQUIRE( index.mapIndex(5) == 6 );
    REQUIRE( index.mapIndex(8) == 9 );

    filters = {{1, {"INFO", "ERR"}}, {2, {"CORE"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 7 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(3) == 6 );
    REQUIRE( index.mapIndex(6) == 9 );

    filters = {{2, {"CORE"}}};
    index.filter(filters);
    REQUIRE( index.getLineCount() == 8 );
    REQUIRE( index.mapIndex(0) == 0 );
    REQUIRE( index.mapIndex(3) == 4 );
    REQUIRE( index.mapIndex(6) == 8 );
}

TEST_CASE("get_values_adjacent") {
    std::string adjacentLog;
    for (int i = 0; i < 10000; ++i) {
        adjacentLog += "10 INFO CORE message 1\n";
    }
    std::stringstream ss(adjacentLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( true );
}
