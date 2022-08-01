project(tests)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(tests
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
    LineColorTests.cpp
    SearcherTests.cpp
    ConfigTests.cpp
    GraphemeMapTests.cpp
    ForeachRangeTests.cpp
    InstanceTrackerTests.cpp
    FilterAlgoTests.cpp
)

target_link_libraries(tests gui)
