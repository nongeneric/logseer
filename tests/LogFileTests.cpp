#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "gui/LogFile.h"
#include "gui/LogTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/Index.h"
#include <QApplication>
#include <condition_variable>
#include <mutex>

using namespace seer;
using namespace gui;

void waitParsingAndIndexing(LogFile& file) {
    REQUIRE(file.isState(gui::sm::IdleState));

    bool parsed = false, indexed = false;

    file.connect(&file, &LogFile::stateChanged, [&] {
        if (file.isState(gui::sm::IndexingState)) {
            parsed = true;
        } else if (file.isState(gui::sm::CompleteState)) {
            indexed = true;
        }
    });

    file.parse();

    while (!parsed) {
        QApplication::processEvents();
    }

    REQUIRE((parsed && !indexed));

    file.index();

    while (!indexed) {
        QApplication::processEvents();
    }

    REQUIRE((parsed && indexed));
}

TEST_CASE("set_is_filter_active") {
    char arg[] = "arg";
    int count = 1; char* args[] = { arg };
    QApplication app(count, args);

    auto ss = std::make_unique<std::stringstream>(simpleLog);
    auto repository = std::make_shared<TestLineParserRepository>();
    LogFile file(std::move(ss), repository);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();

    REQUIRE(model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);

    file.setColumnFilter(1, {"INFO"}); // a single value

    REQUIRE(model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == true);
    REQUIRE(model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);

    file.setColumnFilter(1, {"INFO", "WARN", "ERR"}); // all values

    REQUIRE(model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
    REQUIRE(model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive)
                .toBool() == false);
}
