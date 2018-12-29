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
    auto lineParser = createTestParser();;
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
