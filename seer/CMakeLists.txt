project(seer)

find_package(Qt5Core)
find_library(PCRE2 pcre2-8)
find_library(PCRE2_16 pcre2-16)

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
    InstanceTracker.h
    InstanceTracker.cpp
    task/Task.h
    task/Task.cpp
    task/IndexingTask.h
    task/IndexingTask.cpp
    task/SearchingTask.h
    task/SearchingTask.cpp
    lua/LuaInterpreter.h
    lua/LuaInterpreter.cpp
)

set(LIBS PUBLIC
    Qt5::Core
    ${PCRE2}
    ${PCRE2_16}
    pthread
    ${Boost_LIBRARIES}
    lua
)

if(WIN32)
    list (APPEND LIBS ws2_32)
else()
    find_library(FMT fmt)
    list (APPEND LIBS ${FMT})
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})
