cmake_minimum_required(VERSION 3.11)

project(tests)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Core)

set(SRC
    main.cpp
    ParserTests.cpp
    FilterDialogModelTests.cpp
    LogTableModelTests.cpp
    LogFileTests.cpp
    OffsetIndexTests.cpp
    RandomBitArrayTests.cpp
    TaskTests.cpp
    CommandLineParserTests.cpp
    IndexedEwahTests.cpp
    HistTests.cpp
)
add_executable(tests ${SRC})
target_link_libraries(tests gui)
