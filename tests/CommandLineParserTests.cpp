#include <catch2/catch.hpp>
#include "seer/CommandLineParser.h"

TEST_CASE("command_line_parser_1") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
}

TEST_CASE("command_line_parser_2") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "-v"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( !parser.version().empty() );
    REQUIRE( !parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
}

TEST_CASE("command_line_parser_3") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--version"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( !parser.version().empty() );
    REQUIRE( !parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
}

TEST_CASE("command_line_parser_4") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--help"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( !parser.help().empty() );
    REQUIRE( parser.paths().empty() );
}

TEST_CASE("command_line_parser_5") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--log", "/path/to"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().size() == 1 );
    REQUIRE( parser.paths()[0] == "/path/to" );
}

TEST_CASE("command_line_parser_6") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "/path/to"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().size() == 1 );
    REQUIRE( parser.paths()[0] == "/path/to" );
}

TEST_CASE("command_line_parser_7") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "/path/to",
        "/another/path"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().size() == 2 );
    REQUIRE( parser.paths()[0] == "/path/to" );
    REQUIRE( parser.paths()[1] == "/another/path" );
}

TEST_CASE("command_line_parser_8") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--log", "/path/to",
        "--log", "/another/path"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().size() == 2 );
    REQUIRE( parser.paths()[0] == "/path/to" );
    REQUIRE( parser.paths()[1] == "/another/path" );
}
