#include <catch2/catch.hpp>
#include "TestLineParser.h"
#include "seer/FileParser.h"

using namespace seer;

TEST_CASE("line_color_simple") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    auto context = lineParser->createContext();

    std::vector<std::string> line;
    REQUIRE( lineParser->rgb(line) == 0 ); // empty line
    readAndParseLine(fileParser, 0, line, *context);
    REQUIRE( lineParser->rgb(line) == 0 );
    readAndParseLine(fileParser, 1, line, *context);
    REQUIRE( lineParser->rgb(line) == 0 );
    readAndParseLine(fileParser, 2, line, *context);
    REQUIRE( lineParser->rgb(line) == 0x550000 );
    readAndParseLine(fileParser, 3, line, *context);
    REQUIRE( lineParser->rgb(line) == 0 );
    readAndParseLine(fileParser, 4, line, *context);
    REQUIRE( lineParser->rgb(line) == 0xff0000 );
    readAndParseLine(fileParser, 5, line, *context);
    REQUIRE( lineParser->rgb(line) == 0x550000 );
}
