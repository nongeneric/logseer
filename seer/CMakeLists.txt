cmake_minimum_required(VERSION 3.11)

project(seer)

find_package(Qt5Core)

set(SRC
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
add_library(seer STATIC ${SRC})
target_link_libraries(seer Qt5::Core pcre2-8 pcre2-16 pthread)
