project(seer)

find_package(Qt5Core)

add_library(${PROJECT_NAME} STATIC
    ILineParser.h
    FileParser.h
    FileParser.cpp
    Index.h
    Index.cpp
    Log.h
    Log.cpp
    LineParserRepository.h
    LineParserRepository.cpp
    OffsetIndex.h
    OffsetIndex.cpp
    RandomBitArray.h
    RandomBitArray.cpp
    RegexLineParser.h
    RegexLineParser.cpp
    CommandLineParser.h
    CommandLineParser.cpp
    IndexedEwah.h
    IndexedEwah.cpp
    Hist.h
    Hist.cpp
    Searcher.h
    Searcher.cpp
    task/Task.h
    task/Task.cpp
    task/ParsingTask.h
    task/ParsingTask.cpp
    task/IndexingTask.h
    task/IndexingTask.cpp
    task/SearchingTask.h
    task/SearchingTask.cpp
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Qt5::Core
    pcre2-8
    pcre2-16
    pthread
    ${Boost_LIBRARIES}
)
