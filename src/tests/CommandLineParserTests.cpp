#include <catch2/catch.hpp>
#include "seer/CommandLineParser.h"
#include <filesystem>

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

TEST_CASE("command_line_parser_9") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
    REQUIRE( !parser.verbose() );
    REQUIRE( !parser.fileLog() );
}

TEST_CASE("command_line_parser_10") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--verbose"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
    REQUIRE( parser.verbose() );
    REQUIRE( !parser.fileLog() );
}

TEST_CASE("command_line_parser_11") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--file-log"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.version().empty() );
    REQUIRE( parser.help().empty() );
    REQUIRE( parser.paths().empty() );
    REQUIRE( !parser.verbose() );
    REQUIRE( parser.fileLog() );
}

TEST_CASE("command_line_parser_make_absolute_paths") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "--log", "log",
        "--log", "/tmp/log2"
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.paths().size() == 2 );
    REQUIRE( std::filesystem::path(parser.paths()[0]).is_absolute() );
    REQUIRE( std::filesystem::path(parser.paths()[1]).is_absolute() );
}

TEST_CASE("command_line_parser_make_absolute_paths_2") {
    seer::CommandLineParser parser;
    std::vector<const char*> args {
        "path",
        "log",
    };

    REQUIRE( parser.parse(args.size(), &args[0]) );
    REQUIRE( parser.paths().size() == 1 );
    REQUIRE( std::filesystem::path(parser.paths()[0]).is_absolute() );
}
