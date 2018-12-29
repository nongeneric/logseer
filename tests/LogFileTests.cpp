#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "gui/LogFile.h"
#include "gui/LogTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/Index.h"
#include <seer/task/Task.h>
#include <QApplication>
#include <tbb/concurrent_queue.h>
#include <condition_variable>
#include <mutex>

using namespace seer;
using namespace gui;

namespace {

    class TestTask : public task::Task {
        std::mutex _m;
        std::condition_variable _cv;
        bool _proceed = false;

    protected:
        void body() override {
            auto lock = std::unique_lock(_m);
            auto ms = std::chrono::milliseconds(1);
            while (!_cv.wait_for(lock, ms, [=] { return _proceed; })) {
                if (isStopRequested())
                    return;
                reportProgress(0);
            }
            testBody();
        }

        virtual void testBody() = 0;

    public:
        TestTask() {
            setStateChanged([=] (auto state) {
                this->state = state;
            });
        }

        void proceed() {
            auto lock = std::unique_lock(_m);
            _proceed = true;
            _cv.notify_one();
        }

        std::atomic<task::TaskState> state = task::TaskState::Idle;
    };

    class TestParsingTask : public TestTask {
        FileParser* _fileParser;

    protected:
        void testBody() override {
            _fileParser->index();
        }

    public:
        TestParsingTask(FileParser* fileParser) : _fileParser(fileParser) {}
    };

    class TestIndexingTask : public TestTask {
        Index* _index;
        FileParser* _fileParser;
        ILineParser* _lineParser;

    protected:
        void testBody() override {
            _index->index(_fileParser, _lineParser, [] { return false; });
        }

    public:
        TestIndexingTask(Index* index, FileParser* fileParser, ILineParser* lineParser)
            : _index(index), _fileParser(fileParser), _lineParser(lineParser) {}
    };

    class TestLogFile : public LogFile {

    protected:
        task::Task *createIndexingTask(Index *index, FileParser *fileParser, ILineParser *lineParser) override {
            return indexingTask = new TestIndexingTask(index, fileParser, lineParser);
        }

        task::Task *createParsingTask(FileParser* fileParser) override {
            return parsingTask = new TestParsingTask(fileParser);
        }

    public:
        using LogFile::LogFile;

        TestParsingTask* parsingTask = nullptr;
        TestIndexingTask* indexingTask = nullptr;
    };

} // namespace

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

void waitFor(auto predicate) {
    while (!predicate()) {
        QApplication::processEvents();
    }
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

    file.setColumnFilter(2, {"INFO"}); // a single value

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

    file.setColumnFilter(2, {"INFO", "WARN", "ERR"}); // all values

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

TEST_CASE("headers_should_not_be_clickable_until_file_indexed") {
    char arg[] = "arg";
    int count = 1; char* args[] = { arg };
    QApplication app(count, args);

    auto ss = std::make_unique<std::stringstream>(simpleLog);
    auto repository = std::make_shared<TestLineParserRepository>();
    TestLogFile file(std::move(ss), repository);
    file.parse();

    waitFor([&] { return file.isState(gui::sm::ParsingState); });

    auto model = file.logTableModel();

    int filterRequestedCount = 0;

    QObject::connect(
        &file, &LogFile::filterRequested, [&] { filterRequestedCount++; });

    for (int i = 0; i < g_TestLogColumns + 1; ++i) {
        REQUIRE(model->headerData(i, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                    .toBool() == false);
        file.requestFilter(i);
    }
    REQUIRE( filterRequestedCount == 0 );

    file.parsingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::IndexingState); });

    for (int i = 0; i < g_TestLogColumns + 1; ++i) {
        REQUIRE(model->headerData(i, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                    .toBool() == false);
        file.requestFilter(i);
    }
    REQUIRE( filterRequestedCount == 0 );

    file.indexingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    file.setColumnFilter(2, {});
    file.setColumnFilter(3, {});

    REQUIRE(model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                       .toBool() == false);
    REQUIRE(model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                       .toBool() == false);
    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                       .toBool() == true);
    REQUIRE(model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                       .toBool() == true);
    REQUIRE(model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsIndexed)
                       .toBool() == false);

    for (int i = 0; i < g_TestLogColumns + 1; ++i) {
        file.requestFilter(i);
    }
    REQUIRE( filterRequestedCount == 2 );

    file.interrupt();

    waitFor([&] { return file.isState(gui::sm::InterruptedState); });
}

TEST_CASE("log_file_filtering") {
    char arg[] = "arg";
    int count = 1; char* args[] = { arg };
    QApplication app(count, args);

    auto ss = std::make_unique<std::stringstream>(simpleLog);
    auto repository = std::make_shared<TestLineParserRepository>();
    LogFile file(std::move(ss), repository);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();
    REQUIRE( model->rowCount({}) == 6 );

    file.setColumnFilter(2, {"INFO"});
    REQUIRE( model->rowCount({}) == 3 );
}
