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

TEST_CASE("model_selection") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );

    auto [first, last] = model.getSelection();
    REQUIRE( first == -1 );
    REQUIRE( last == -1 );

    model.setSelection(1);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 2 );

    model.extendSelection(3);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 4 );
    REQUIRE( model.isSelected(1) );
    REQUIRE( model.isSelected(2) );
    REQUIRE( model.isSelected(3) );
    REQUIRE( !model.isSelected(4) );

    model.setSelection(3);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 3 );
    REQUIRE( last == 4 );
    REQUIRE( model.isSelected(3) );
    REQUIRE( !model.isSelected(4) );

    model.extendSelection(1);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 4 );
    REQUIRE( model.isSelected(1) );
    REQUIRE( model.isSelected(2) );
    REQUIRE( model.isSelected(3) );
}

TEST_CASE("model_selection_extending_past_last_line") {
    std::stringstream ss(simpleLog);
    auto lineParser = createTestParser();
    FileParser fileParser(&ss, lineParser.get());
    fileParser.index();

    Index index;
    index.index(&fileParser, lineParser.get(), []{ return false; }, [](auto, auto){});
    LogTableModel model(&fileParser);

    REQUIRE( model.rowCount({}) == 6 );

    model.setSelection(1);
    model.extendSelection(100);
    auto [first, last] = model.getSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 6 );

    model.setSelection(3);
    model.extendSelection(-50);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 0 );
    REQUIRE( last == 4 );
}

TEST_CASE("model_selection_extending_without_previous_selection") {
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

    model.extendSelection(100);
    auto [first, last] = model.getSelection();
    REQUIRE( first == -1 );
    REQUIRE( last == -1 );
    REQUIRE( changed == 0 );

    model.setSelection(1);
    REQUIRE( changed == 1 );

    model.extendSelection(100);
    std::tie(first, last) = model.getSelection();
    REQUIRE( first == 1 );
    REQUIRE( last == 6 );
    REQUIRE( changed == 2 );
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
