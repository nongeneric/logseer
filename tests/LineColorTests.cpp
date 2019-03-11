#include <catch2/catch.hpp>
#include "TestLineParser.h"
#include "seer/FileParser.h"

using namespace seer;

TEST_CASE("line_color_simple") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser(ss);
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    std::vector<std::string> line;
    REQUIRE( lineParser->rgb(line) == 0 ); // empty line
    fileParser.readLine(0, line);
    REQUIRE( lineParser->rgb(line) == 0 );
    fileParser.readLine(1, line);
    REQUIRE( lineParser->rgb(line) == 0 );
    fileParser.readLine(2, line);
    REQUIRE( lineParser->rgb(line) == 0x550000 );
    fileParser.readLine(3, line);
    REQUIRE( lineParser->rgb(line) == 0 );
    fileParser.readLine(4, line);
    REQUIRE( lineParser->rgb(line) == 0xff0000 );
    fileParser.readLine(5, line);
    REQUIRE( lineParser->rgb(line) == 0x550000 );
}
