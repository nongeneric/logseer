cmake_minimum_required (VERSION 3.15)

project(logseer VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file (
    "${CMAKE_SOURCE_DIR}/version.h.in"
    "${CMAKE_BINARY_DIR}/version.h"
)

include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories(SYSTEM ../thirdparty/Catch2/single_include)
include_directories(SYSTEM ../thirdparty/EWAHBoolArray/include)
include_directories(SYSTEM ../thirdparty/sml/include/boost)
include_directories(SYSTEM ../thirdparty/concurrentqueue)
include_directories(SYSTEM ../thirdparty/json/include)

file(GLOB LUA_SOURCES ../thirdparty/lua/*.c)
add_library(lua STATIC $<FILTER:${LUA_SOURCES},EXCLUDE,onelua.c>)
target_include_directories(lua INTERFACE SYSTEM ../thirdparty/lua)

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.56 COMPONENTS system program_options REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_library(FMT fmt)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Werror>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall>)

if(WIN32)
    add_compile_definitions(WIN32)
endif()

add_subdirectory(gui)
add_subdirectory(seer)
add_subdirectory(tests)

if(WIN32)
    configure_file (
        "${CMAKE_SOURCE_DIR}/install.cmake.in"
        "${CMAKE_BINARY_DIR}/install.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_BINARY_DIR}/install.cmake")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-win64")
    set(CPACK_GENERATOR "ZIP")
    include(CPack)
else()
    install(TARGETS logseer DESTINATION bin)
endif()
