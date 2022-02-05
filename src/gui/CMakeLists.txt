project(gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Core)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-Wl,-subsystem,windows ${CMAKE_CXX_FLAGS}")
endif()

add_library(gui STATIC
    MainWindow.h
    MainWindow.cpp
    LogTableModel.h
    LogTableModel.cpp
    FilterDialog.h
    FilterDialog.cpp
    FilterTableModel.h
    FilterTableModel.cpp
    LogFile.h
    LogFile.cpp
    SearchLine.h
    SearchLine.cpp
    grid/FilterHeaderView.h
    grid/FilterHeaderView.cpp
    grid/LogTable.h
    grid/LogTable.cpp
    grid/LogTableView.h
    grid/LogTableView.cpp
    grid/LogScrollArea.h
    grid/LogScrollArea.cpp
    grid/HistMap.h
    grid/HistMap.cpp
    grid/GotoLineDialog.h
    grid/GotoLineDialog.cpp
    sm/Logger.h
    sm/StateMachine.h
    ThreadDispatcher.h
    ThreadDispatcher.cpp
    Config.h
    Config.cpp
    GraphemeMap.h
    GraphemeMap.cpp
)

target_link_libraries(gui
    PUBLIC
    Qt5::Core
    Qt5::Widgets
    seer
)

target_compile_definitions(gui PUBLIC QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

add_executable(logseer main.cpp ${QT_RESOURCES} resources/resources.qrc "${CMAKE_CURRENT_SOURCE_DIR}/resources/winicon/icon.rc")
target_link_libraries(logseer gui)