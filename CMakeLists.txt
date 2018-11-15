cmake_minimum_required (VERSION 3.11)

project(logseer)

set(CMAKE_CXX_STANDARD 17)

set(logseer_VERSION "0.0.1")

configure_file (
    "${CMAKE_SOURCE_DIR}/version.h.in"
    "${CMAKE_BINARY_DIR}/version.h"
)

include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories(SYSTEM thirdparty/Catch2/single_include)
include_directories(SYSTEM thirdparty/EWAHBoolArray/headers)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.50 COMPONENTS system program_options REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

option(CMAKE_RELEASE "CMAKE_RELEASE" FALSE)

set(CMAKE_CXX_FLAGS "-Werror=return-type -Wall -Wextra -Werror -Wno-implicit-fallthrough ${CMAKE_CXX_FLAGS}")

if(CMAKE_RELEASE)
    add_definitions(-DNDEBUG)
    set(CMAKE_CXX_FLAGS "-O0 ${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "-O0 -ggdb ${CMAKE_CXX_FLAGS}")
endif()

add_subdirectory(gui)
add_subdirectory(seer)
if(NOT CMAKE_RELEASE)
    add_subdirectory(tests)
endif()

if(WIN32)
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_PACKAGE_FILE_NAME "logseer-${logseer_VERSION}-win64")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
endif()
