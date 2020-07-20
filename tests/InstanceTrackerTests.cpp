#include <catch2/catch.hpp>

#include "seer/InstanceTracker.h"

TEST_CASE("instance_tracker_simple") {
    auto socket = "logseer_tests.socket";

    seer::InstanceTracker primaryTracker(socket);
    REQUIRE( !primaryTracker.connected() );

    seer::InstanceTracker secondTracker(socket);
    REQUIRE( secondTracker.connected() );

    secondTracker.send("message");
    std::string message;
    message = *primaryTracker.waitMessage();
    REQUIRE( message == "message" );

    seer::InstanceTracker thirdTracker(socket);
    REQUIRE( thirdTracker.connected() );

    thirdTracker.send("another message");
    message = *primaryTracker.waitMessage();
    REQUIRE( message == "another message" );

    primaryTracker.stop();
    REQUIRE( !primaryTracker.waitMessage().has_value() );
}
