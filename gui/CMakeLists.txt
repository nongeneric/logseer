cmake_minimum_required(VERSION 3.11)

project(gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Core)

if(WIN32)
    set(CMAKE_CXX_FLAGS "-Wl,-subsystem,windows ${CMAKE_CXX_FLAGS}")
endif()

set(SRC
    MainWindow.h
    MainWindow.cpp
    MainWindowViewModel.h
    MainWindowViewModel.cpp
    LogTableModel.h
    LogTableModel.cpp
    FilterDialog.h
    FilterDialog.cpp
    FilterTableModel.h
    FilterTableModel.cpp
    LogFile.h
    LogFile.cpp
    grid/FilterHeaderView.h
    grid/FilterHeaderView.cpp
)
add_library(gui STATIC ${SRC})
target_link_libraries(gui Qt5::Core Qt5::Widgets seer)
add_executable(logseer main.cpp ${QT_RESOURCES} resources/resources.qrc)
target_link_libraries(logseer gui ${Boost_LIBRARIES})
