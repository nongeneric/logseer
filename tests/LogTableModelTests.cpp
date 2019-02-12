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
    auto lineParser = createTestParser(ss);
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
    auto lineParser = createTestParser(ss);
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
    auto lineParser = createTestParser(ss);
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
