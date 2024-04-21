project(gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(gui STATIC
    MainWindow.cpp
    LogTableModel.cpp
    FilterDialog.cpp
    FilterTableModel.cpp
    LogFile.cpp
    SearchLine.cpp
    grid/FilterHeaderView.cpp
    grid/LogTable.cpp
    grid/LogTableView.cpp
    grid/LogScrollArea.cpp
    grid/HistMap.cpp
    grid/GotoLineDialog.cpp
    grid/CachedHighlightSearcher.cpp
    ThreadDispatcher.cpp
    Config.cpp
    GraphemeMap.cpp
)

target_link_libraries(gui
    PUBLIC
    Qt6::Core
    Qt6::Widgets
    seer
)

add_executable(logseer WIN32 main.cpp ${QT_RESOURCES} resources/resources.qrc "${CMAKE_CURRENT_SOURCE_DIR}/resources/winicon/icon.rc")
target_link_libraries(logseer gui)

install(TARGETS logseer DESTINATION /)
install(FILES $<TARGET_RUNTIME_DLLS:logseer> DESTINATION /)
