cmake_minimum_required(VERSION 3.11)

project(seer)

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
target_link_libraries(seer pthread)
