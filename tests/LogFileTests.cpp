#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "gui/LogTableModel.h"
#include "gui/LogFile.h"
#include "seer/ILineParser.h"
#include "seer/FileParser.h"
#include "seer/Index.h"
#include <mutex>
#include <condition_variable>

using namespace seer;
using namespace gui;

void waitParsingAndIndexing(LogFile& file) {
    REQUIRE( file.state() == gui::LogFileState::Idle );

    std::mutex m;
    std::condition_variable cv;

    bool parsed = false, indexed = false;

    file.connect(&file, &LogFile::parsingComplete, [&] {
        auto lock = std::lock_guard(m);
        cv.notify_all();
        parsed = true;
    });

    file.connect(&file, &LogFile::indexingComplete, [&] {
        auto lock = std::lock_guard(m);
        cv.notify_all();
        indexed = true;
    });

    file.parse();
    {
        auto lock = std::unique_lock(m);
        cv.wait(lock, [&] { return parsed; });
    }

    REQUIRE( (parsed && !indexed) );

    file.index();
    {
        auto lock = std::unique_lock(m);
        cv.wait(lock, [&] { return indexed; });
    }

    REQUIRE( (parsed && indexed) );
}

TEST_CASE("set_is_filter_active") {
    auto ss = std::make_unique<std::stringstream>(simpleLog);
    auto repository = std::make_shared<TestLineParserRepository>();
    LogFile file(std::move(ss), repository);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();

    REQUIRE( model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );

    file.setColumnFilter(1, {"INFO"}); // a single value

    REQUIRE( model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == true );
    REQUIRE( model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );

    file.setColumnFilter(1, {"INFO", "WARN", "ERR"}); // all values

    REQUIRE( model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
    REQUIRE( model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false );
}
