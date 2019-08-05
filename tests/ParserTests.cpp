#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "seer/ILineParser.h"
#include "seer/FileParser.h"
#include "seer/Index.h"
#include "seer/LineParserRepository.h"
#include <sstream>

using namespace seer;

TEST_CASE("get_parser_name") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    REQUIRE(lineParser->name() == "test parser");
}

TEST_CASE("simple_parser") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
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
    auto lineParser = createTestParser(ss);
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
    auto lineParser = createTestParser(ss);
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
    REQUIRE( values[0].value == "CORE" );
    REQUIRE( values[1].value == "SUB" );
    REQUIRE( values[0].count == 3 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    index.filter(filters);
    values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[0].count == 1 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[2].count == 2 );
    REQUIRE( values[0].checked == false );
    REQUIRE( values[1].checked == true );
    REQUIRE( values[2].checked == false );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0].count == 1 );
    REQUIRE( values[1].count == 2 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );
}

TEST_CASE("get_values_three_columns") {
    std::stringstream ss(threeColumnLog);
    auto lineParser = createThreeColumnTestParser(ss);
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
    REQUIRE( values[0].value == "CORE" );
    REQUIRE( values[1].value == "SUB" );
    REQUIRE( values[0].count == 3 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );

    values = index.getValues(3);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0].value == "F1" );
    REQUIRE( values[1].value == "F2" );
    REQUIRE( values[0].count == 3 );
    REQUIRE( values[1].count == 3 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );

    std::vector<ColumnFilter> filters;
    filters = {{1, {"ERR", "WARN"}},
               {2, {"CORE"}},
               {3, {"F1", "F2"}}};
    index.filter(filters);
    values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[0].value == "ERR" );
    REQUIRE( values[1].value == "INFO" );
    REQUIRE( values[2].value == "WARN" );
    REQUIRE( values[0].count == 1 );
    REQUIRE( values[1].count == 1 );
    REQUIRE( values[2].count == 1 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == false );
    REQUIRE( values[2].checked == true );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0].value == "CORE" );
    REQUIRE( values[1].value == "SUB" );
    REQUIRE( values[0].count == 2 );
    REQUIRE( values[1].count == 1 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == false );

    values = index.getValues(3);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[0].value == "F1" );
    REQUIRE( values[1].value == "F2" );
    REQUIRE( values[0].count == 2 );
    REQUIRE( values[1].count == 0 );
    REQUIRE( values[0].checked == true );
    REQUIRE( values[1].checked == true );
}

TEST_CASE("search") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
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

    seer::Hist hist(1);

    indexCopy.search(&fileParser, "4", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );

    indexCopy = index;
    indexCopy.search(&fileParser, "4", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 2 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );
    REQUIRE( indexCopy.mapIndex(1) == 5 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 0 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", false, false, false, hist);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 3 );
}

TEST_CASE("search_regex") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
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

    seer::Hist hist(1);

    indexCopy.search(&fileParser, "4|9", true, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );

    indexCopy = index;
    indexCopy.search(&fileParser, "4\\d", true, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 5 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", true, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 0 );

    indexCopy = index;
    indexCopy.search(&fileParser, "inf", true, false, false, hist);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 3 );
}

TEST_CASE("search_multiline") {
    std::stringstream ss(multilineFirstLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();
    REQUIRE( fileParser.lineCount() == 8 );

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    REQUIRE( index.getLineCount() == 8 );

    seer::Hist hist(1);

    auto indexCopy = index;
    indexCopy.search(&fileParser, "message 1", true, false, true, hist);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 2 );

    indexCopy = index;
    indexCopy.search(&fileParser, "message 5", true, false, true, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 4 );
}

TEST_CASE("search_progress") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    seer::Hist hist(1);

    std::vector<std::tuple<uint64_t, uint64_t>> progress;

    auto indexCopy = index;
    indexCopy.search(
        &fileParser,
        "4",
        false,
        false,
        false,
        hist,
        [] { return false; },
        [&](auto i, auto j) {
            progress.push_back({i, j});
        });
    REQUIRE( progress.size() == 6 );
    REQUIRE( progress[0] == std::tuple{0,6} );
    REQUIRE( progress[1] == std::tuple{1,6} );
    REQUIRE( progress[2] == std::tuple{2,6} );
    REQUIRE( progress[3] == std::tuple{3,6} );
    REQUIRE( progress[4] == std::tuple{4,6} );
    REQUIRE( progress[5] == std::tuple{5,6} );

    indexCopy = index;
    progress.clear();
    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    indexCopy.filter(filters);
    indexCopy.search(
        &fileParser,
        "4",
        false,
        false,
        false,
        hist,
        [] { return false; },
        [&](auto i, auto j) {
            progress.push_back({i, j});
        });
    REQUIRE( progress.size() == 3 );
    REQUIRE( progress[0] == std::tuple{0,6} );
    REQUIRE( progress[1] == std::tuple{1,6} );
    REQUIRE( progress[2] == std::tuple{3,6} );
}


TEST_CASE("multiline_index") {
    std::stringstream ss(multilineLog);
    auto lineParser = createTestParser(ss);
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
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( true );
}

TEST_CASE("get_values_counts") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    /*
        10 INFO CORE message 1
        15 INFO SUB  message 2
        17 WARN CORE message 3
        20 INFO SUB  message 4
        30 ERR  CORE message 5
        40 WARN SUB  message 6

        column 1 values: ERROR INFO WARN
        column 2 values: CORE SUB
    */

    const auto ERROR = 0;
    const auto INFO = 1;
    const auto WARN = 2;
    const auto CORE = 0;
    const auto SUB = 1;

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    auto values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[ERROR].count == 1 );
    REQUIRE( values[INFO].count == 3 );
    REQUIRE( values[WARN].count == 2 );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[CORE].count == 3 );
    REQUIRE( values[SUB].count == 3 );

    /*
        10 INFO CORE message 1
        15 INFO SUB  message 2
        20 INFO SUB  message 4
    */

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    index.filter(filters);
    values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[ERROR].count == 1 );
    REQUIRE( values[INFO].count == 3 );
    REQUIRE( values[WARN].count == 2 );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[CORE].count == 1 );
    REQUIRE( values[SUB].count == 2 );

    /*
        10 INFO CORE message 1
        17 WARN CORE message 3
        30 ERR  CORE message 5
    */

    filters = {{2, {"CORE"}}};
    index.filter(filters);
    values = index.getValues(1);
    REQUIRE( values.size() == 3 );
    REQUIRE( values[ERROR].count == 1 );
    REQUIRE( values[INFO].count == 1 );
    REQUIRE( values[WARN].count == 1 );

    values = index.getValues(2);
    REQUIRE( values.size() == 2 );
    REQUIRE( values[CORE].count == 3 );
    REQUIRE( values[SUB].count == 3 );
}

TEST_CASE("index_column_width") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( index.maxWidth(0) == 2 );
    REQUIRE( index.maxWidth(1) == 4 );
    REQUIRE( index.maxWidth(2) == 4 );
    REQUIRE( index.maxWidth(3) == 9 );
}

TEST_CASE("multiline_index_column_width") {
    std::stringstream ss(multilineLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( index.maxWidth(0) == 2 );
    REQUIRE( index.maxWidth(1) == 4 );
    REQUIRE( index.maxWidth(2) == 4 );
    REQUIRE( index.maxWidth(3) == 11 );
}

TEST_CASE("search_hist_simple") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    /*
        10 INFO CORE message 1
        15 INFO SUB message 2
        17 WARN CORE message 3
        20 INFO SUB message 4
        30 ERR CORE message 5
        40 WARN SUB message 6
    */

    Hist hist(1000);
    auto indexCopy = index;

    /*
        10 INFO CORE message 1
        15 INFO SUB message 2
        17 WARN CORE message 3
      + 20 INFO SUB message 4
        30 ERR CORE message 5
      + 40 WARN SUB message 6
    */

    indexCopy.search(&fileParser, "4", false, true, false, hist);
    REQUIRE( hist.get(0, 6) == 0 );
    REQUIRE( hist.get(1, 6) == 0 );
    REQUIRE( hist.get(2, 6) == 0 );
    REQUIRE( hist.get(3, 6) == 1 );
    REQUIRE( hist.get(4, 6) == 0 );
    REQUIRE( hist.get(5, 6) == 1 );

    /*
        10 INFO CORE message 1
        15 INFO SUB message 2
      + 20 INFO SUB message 4
    */

    std::vector<ColumnFilter> filters;
    filters = {{1, {"INFO"}}};
    indexCopy = index;
    Hist hist2(1000);
    indexCopy.filter(filters);
    indexCopy.search(&fileParser, "4", false, true, false, hist2);
    REQUIRE( hist2.get(0, 3) == 0 );
    REQUIRE( hist2.get(1, 3) == 0 );
    REQUIRE( hist2.get(2, 3) == 1 );
}

TEST_CASE("search_unicode") {
    std::stringstream ss(unicodeLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    /*
        10 ИНФО CORE message 1
        15 ИНФО SUB message 2
        17 WARN CORE message 3
        20 ИНФО SUB GRÜẞEN 4
        30 ERR CORE GRÜSSEN 5
        40 WARN SUB grüßen 6
    */

    auto indexCopy = index;
    std::vector<ColumnFilter> filters;
    filters = {{1, {u8"ИНФО"}}};
    indexCopy.filter(filters);
    REQUIRE( indexCopy.getLineCount() == 3 );
    REQUIRE( indexCopy.mapIndex(0) == 0 );
    REQUIRE( indexCopy.mapIndex(1) == 1 );
    REQUIRE( indexCopy.mapIndex(2) == 3 );

    seer::Hist hist(1);

    indexCopy = index;
    indexCopy.search(&fileParser, u8"grüßen", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 5 );

    indexCopy = index;
    indexCopy.search(&fileParser, u8"grüßen", false, false, false, hist);
    REQUIRE( indexCopy.getLineCount() == 2 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );
    REQUIRE( indexCopy.mapIndex(1) == 5 );

    indexCopy = index;
    indexCopy.search(&fileParser, u8"GRÜSSEN", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 4 );

    indexCopy = index;
    indexCopy.search(&fileParser, u8"GRÜSSEN", false, false, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 4 );

    indexCopy = index;
    indexCopy.search(&fileParser, u8"GRÜẞEN", false, true, false, hist);
    REQUIRE( indexCopy.getLineCount() == 1 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );

    indexCopy = index;
    indexCopy.search(&fileParser, u8"GRÜẞEN", false, false, false, hist);
    REQUIRE( indexCopy.getLineCount() == 2 );
    REQUIRE( indexCopy.mapIndex(0) == 3 );
    REQUIRE( indexCopy.mapIndex(1) == 5 );
}

TEST_CASE("default_line_parser") {
    seer::LineParserRepository repository;
    std::stringstream ss("abc");
    auto lineParser = repository.resolve(ss);
    REQUIRE( lineParser->name() == "default" );
}

TEST_CASE("default_line_parser_columns") {
    seer::LineParserRepository repository;
    std::stringstream ss(unstructuredLog);
    auto lineParser = repository.resolve(ss);
    REQUIRE( lineParser->name() == "default" );

    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();
    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( index.getLineCount() == 4 );
    REQUIRE( fileParser.lineCount() == 4 );
    std::string line;
    fileParser.readLine(0, line);
    std::vector<std::string> columns;
    REQUIRE( line == "message1" );
    fileParser.readLine(0, columns);
    REQUIRE( columns.size() == 1 );
    REQUIRE( columns[0] == "message1" );

    REQUIRE( index.maxWidth(0) == std::string("message1").size() );
}

TEST_CASE("parse_zeroes") {
    seer::LineParserRepository repository;
    std::stringstream ss(zeroesLog);
    auto lineParser = repository.resolve(ss);
    REQUIRE( lineParser->name() == "default" );

    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();
    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});

    REQUIRE( index.getLineCount() == 1 );
    REQUIRE( fileParser.lineCount() == 1 );
    std::string line;
    fileParser.readLine(0, line);
    std::vector<std::string> columns;
    REQUIRE( line.size() == std::string("message1").size() );
    REQUIRE( line == "message1" );
    fileParser.readLine(0, columns);
    REQUIRE( columns.size() == 1 );
    REQUIRE( columns[0] == "message1" );

    REQUIRE( index.maxWidth(0) == std::string("message1").size() );

    // cached last line size
    line = "abc";
    fileParser.readLine(0, line);
    REQUIRE( line.size() == std::string("message1").size() );
    REQUIRE( line == "message1" );
}

TEST_CASE("parse_autosize_attribute") {
    std::stringstream ss(threeColumnLog);
    auto lineParser = createThreeColumnTestParser(ss);
    auto columns = lineParser->getColumnFormats();
    REQUIRE( columns[0].autosize == true );
    REQUIRE( columns[1].autosize == true );
    REQUIRE( columns[2].autosize == false );
    REQUIRE( columns[3].autosize == false );
    REQUIRE( columns[4].autosize == false );
}
