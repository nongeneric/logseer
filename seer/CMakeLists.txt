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
)
add_library(seer STATIC ${SRC})
target_link_libraries(seer pthread)
