#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "gui/LogTableModel.h"
#include "seer/ILineParser.h"
#include "seer/FileParser.h"
#include "seer/Index.h"

using namespace seer;
using namespace gui;

TEST_CASE("return_roles") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);
    model.showIndexedColumns();
    model.setIndex(&index);
    REQUIRE( model.headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool() == false );
    REQUIRE( model.headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool() == false );
    REQUIRE( model.headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool() == true );
    REQUIRE( model.headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool() == true );
    REQUIRE( model.headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsIndexed).toBool() == false );

    REQUIRE( model.headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );

    model.setFilterActive(0, true);
    model.setFilterActive(3, true);

    REQUIRE( model.headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == true );
    REQUIRE( model.headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model.headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == true );
    REQUIRE( model.headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
}

TEST_CASE("model_row_selection") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );

    auto [first, last] = model.getRowSelection();
    REQUIRE( first == -1 );
    REQUIRE( last == -1 );

    model.setSelection(1, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 1 );

    model.extendSelection(3, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 3 );

    model.setSelection(3, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 3 );
    REQUIRE( last == 3 );

    model.extendSelection(1, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 3 );
}

TEST_CASE("model_row_selection_extending_past_last_line") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );

    model.setSelection(1, 0, 0);
    model.extendSelection(100, 0, 0);
    auto [first, last] = model.getRowSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 5 );

    model.setSelection(3, 0, 0);
    model.extendSelection(-50, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 0 );
    REQUIRE( last == 3 );
}

TEST_CASE("model_row_selection_extending_without_previous_selection") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );

    int changed = 0;
    model.connect(&model, &LogTableModel::selectionChanged, [&] {
        changed++;
    });

    model.extendSelection(100, 0, 0);
    auto [first, last] = model.getRowSelection();
    REQUIRE( first == -1 );
    REQUIRE( last == -1 );
    REQUIRE( changed == 0 );

    model.setSelection(1, 0, 0);
    REQUIRE( changed == 1 );

    model.extendSelection(100, 0, 0);
    std::tie(first, last) = model.getRowSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 5 );
    REQUIRE( changed == 2 );
}

TEST_CASE("model_selection") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );
    REQUIRE( std::holds_alternative<std::monostate>(model.getSelection()) );

    model.setSelection(1, 2, 1); // 15 I*NFO SUB message 2
    auto rowSelection = std::get<RowSelection>(model.getSelection());
    REQUIRE( rowSelection.first == 1 );
    REQUIRE( rowSelection.last == 1 );

    SECTION("extending to the right") {
        model.extendSelection(1, 2, 2); // 15 IN*FO SUB message 2

        auto columnSelection = std::get<ColumnSelection>(model.getSelection());
        REQUIRE( columnSelection.row == 1 );
        REQUIRE( columnSelection.column == 2 );
        REQUIRE( columnSelection.first == 1 );
        REQUIRE( columnSelection.last == 2 );
    }

    SECTION("extending past the start of column") {
        model.extendSelection(1, 0, 0); // *<line num> 15 INFO SUB message 2

        auto columnSelection = std::get<ColumnSelection>(model.getSelection());
        REQUIRE( columnSelection.row == 1 );
        REQUIRE( columnSelection.column == 2 );
        REQUIRE( columnSelection.first == 0 );
        REQUIRE( columnSelection.last == 1 );
    }

    SECTION("extending past the end of column") {
        model.extendSelection(1, 3, 2); // 15 INFO SU*B message 2

        auto columnSelection = std::get<ColumnSelection>(model.getSelection());
        REQUIRE( columnSelection.row == 1 );
        REQUIRE( columnSelection.column == 2 );
        REQUIRE( columnSelection.first == 1 );
        REQUIRE( columnSelection.last == 3 );
    }

    SECTION("extending outside the row up") {
        model.extendSelection(0, 0, 0);

        auto rowSelection = std::get<RowSelection>(model.getSelection());
        REQUIRE( rowSelection.first == 0 );
        REQUIRE( rowSelection.last == 1 );

        SECTION("extending back to the original row") {
            model.extendSelection(1, 3, 2); // 15 INFO SU*B message 2

            auto columnSelection = std::get<ColumnSelection>(model.getSelection());
            REQUIRE( columnSelection.row == 1 );
            REQUIRE( columnSelection.column == 2 );
            REQUIRE( columnSelection.first == 1 );
            REQUIRE( columnSelection.last == 3 );
        }
    }

    SECTION("extending outside the row down") {
        model.extendSelection(3, 0, 0);

        auto rowSelection = std::get<RowSelection>(model.getSelection());
        REQUIRE( rowSelection.first == 1 );
        REQUIRE( rowSelection.last == 3 );

        SECTION("extending back to the original row") {
            model.extendSelection(1, 3, 2); // 15 INFO SU*B message 2

            auto columnSelection = std::get<ColumnSelection>(model.getSelection());
            REQUIRE( columnSelection.row == 1 );
            REQUIRE( columnSelection.column == 2 );
            REQUIRE( columnSelection.first == 1 );
            REQUIRE( columnSelection.last == 3 );
        }
    }

    SECTION("extending the same char") {
        model.setSelection(1, 2, 1);

        auto rowSelection = std::get<RowSelection>(model.getSelection());
        REQUIRE( rowSelection.first == 1 );
        REQUIRE( rowSelection.last == 1 );
    }
}

TEST_CASE("copy_raw_lines") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    std::vector<std::string> expected {
        "15 INFO SUB message 2",
        "17 WARN CORE message 3"
    };

    std::vector<std::string> actual;

    model.copyRawLines(1, 3, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("copy_lines") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    std::vector<std::string> expected {
        "#   Timestamp   Level   Component   Message",
        "---------------------------------------------",
        "2   15          INFO    SUB         message 2",
        "3   17          WARN    CORE        message 3"
    };

    std::vector<std::string> actual;

    model.copyLines(1, 3, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("copy_lines_multiline") {
    std::stringstream ss(multilineLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    std::vector<std::string> expected {
        "#   Timestamp   Level   Component   Message",
        "-----------------------------------------------",
        "1   10          INFO    CORE        message 1",
        "                                    message 1 a",
        "                                    message 1 b",
        "4   15          INFO    SUB         message 2",
    };

    std::vector<std::string> actual;

    model.copyLines(0, 4, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("copy_lines_line_number_spacing") {
    std::stringstream ss(simpleLog + simpleLog + simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    std::vector<std::string> expected {
        "#    Timestamp   Level   Component   Message",
        "----------------------------------------------",
        "11   30          ERR     CORE        message 5",
    };

    std::vector<std::string> actual;

    model.copyLines(10, 11, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}
