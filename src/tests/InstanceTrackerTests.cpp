#include <catch2/catch.hpp>

#include "seer/InstanceTracker.h"
#include <filesystem>

TEST_CASE("instance_tracker_simple") {
    auto socket = "logseer_tests.socket";

    seer::InstanceTracker primaryTracker(socket);
    REQUIRE( !primaryTracker.connected() );

    seer::InstanceTracker secondTracker(socket);
    REQUIRE( secondTracker.connected() );

    secondTracker.send("message");
    std::string message;
    message = *primaryTracker.waitMessage();
    REQUIRE( std::filesystem::path(message).stem() == "message" );

    seer::InstanceTracker thirdTracker(socket);
    REQUIRE( thirdTracker.connected() );

    thirdTracker.send("another message");
    message = *primaryTracker.waitMessage();
    REQUIRE( std::filesystem::path(message).stem() == "another message" );

    primaryTracker.stop();
    REQUIRE( !primaryTracker.waitMessage().has_value() );
}

TEST_CASE("instance_tracker_make_absolute_filepath") {
    auto socket = "logseer_tests.socket";

    seer::InstanceTracker primaryTracker(socket);
    REQUIRE( !primaryTracker.connected() );

    seer::InstanceTracker secondTracker(socket);
    REQUIRE( secondTracker.connected() );

    secondTracker.send("file");
    auto message = *primaryTracker.waitMessage();
    REQUIRE( std::filesystem::path(message).is_absolute() );
    REQUIRE( std::filesystem::path(message).stem() == "file" );

    primaryTracker.stop();
}
