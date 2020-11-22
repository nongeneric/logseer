#include <catch2/catch.hpp>

#include "TestLineParser.h"
#include "gui/LogFile.h"
#include "gui/LogTableModel.h"
#include "seer/FileParser.h"
#include "seer/ILineParser.h"
#include "seer/Index.h"
#include "seer/task/Task.h"
#include "seer/StriingLiterals.h"
#include <QApplication>
#include <condition_variable>
#include <mutex>
#include <QBrush>
#include <fstream>

using namespace seer;
using namespace gui;

namespace {

QApplication* qapp() {
    static int count = 0;
    static auto app = std::make_unique<QApplication>(count, nullptr);
    return app.get();
}

class TestTask : public task::Task {
    std::mutex _m;
    std::condition_variable _cv;
    bool _proceed = false;

protected:
    void body() override {
        auto lock = std::unique_lock(_m);
        auto ms = std::chrono::milliseconds(1);
        while (!_cv.wait_for(lock, ms, [this] { return _proceed; })) {
            if (isStopRequested())
                return;
            reportProgress(0);
        }
        testBody();
    }

    virtual void testBody() = 0;

public:
    TestTask() {
        setStateChanged([this] (auto state) {
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
    std::shared_ptr<task::Task> createIndexingTask(Index *index, FileParser *fileParser, ILineParser *lineParser) override {
        return indexingTask = std::make_shared<TestIndexingTask>(index, fileParser, lineParser);
    }

    std::shared_ptr<task::Task> createParsingTask(FileParser* fileParser) override {
        return parsingTask = std::make_shared<TestParsingTask>(fileParser);
    }

public:
    using LogFile::LogFile;

    std::shared_ptr<TestParsingTask> parsingTask = nullptr;
    std::shared_ptr<TestIndexingTask> indexingTask = nullptr;
};

} // namespace

template <class P>
void waitFor(P predicate) {
    while (!predicate()) {
        QApplication::processEvents();
    }
}

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

    waitFor([&] { return parsed; });

    REQUIRE((parsed && !indexed));

    file.index();

    waitFor([&] { return indexed; });

    REQUIRE((parsed && indexed));

    file.disconnect();
}

template <class T = LogFile>
T makeLogFile(std::string log, std::string parserName = "") {
    auto ss = std::make_unique<std::stringstream>(log);
    auto repository = std::make_shared<TestLineParserRepository>();

    std::shared_ptr<ILineParser> lineParser;
    if (parserName.empty()) {
        lineParser = repository->resolve(*ss);
    } else {
        lineParser = resolveByName(repository.get(), parserName);
    }

    return T(std::move(ss), lineParser);
}

TEST_CASE("set_is_filter_active") {
    qapp();

    auto file = makeLogFile(simpleLog);
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
    qapp();

    auto file = makeLogFile<TestLogFile>(simpleLog);
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
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();
    REQUIRE( model->rowCount({}) == 6 );

    file.setColumnFilter(2, {"INFO"});
    REQUIRE( model->rowCount({}) == 3 );
}

TEST_CASE("log_file_multiline") {
    qapp();

    auto file = makeLogFile(multilineLog);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();
    REQUIRE( model->rowCount({}) == 11 );

    REQUIRE(model->data(model->index(0, 0), Qt::DisplayRole).toString() == "1");
    REQUIRE(model->data(model->index(0, 1), Qt::DisplayRole).toString() == "10");
    REQUIRE(model->data(model->index(0, 2), Qt::DisplayRole).toString() == "INFO");
    REQUIRE(model->data(model->index(0, 3), Qt::DisplayRole).toString() == "CORE");
    REQUIRE(model->data(model->index(0, 4), Qt::DisplayRole).toString() == "message 1");

    REQUIRE(model->data(model->index(1, 0), Qt::DisplayRole).toString() == "2");
    REQUIRE(model->data(model->index(1, 1), Qt::DisplayRole).toString() == "");
    REQUIRE(model->data(model->index(1, 2), Qt::DisplayRole).toString() == "");
    REQUIRE(model->data(model->index(1, 3), Qt::DisplayRole).toString() == "");
    REQUIRE(model->data(model->index(1, 4), Qt::DisplayRole).toString() == "message 1 a");
}

TEST_CASE("count_lines_from_one") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();
    REQUIRE( model->rowCount({}) == 6 );

    REQUIRE(model->data(model->index(0, 0), Qt::DisplayRole).toString() == "1");
    REQUIRE(model->data(model->index(1, 0), Qt::DisplayRole).toString() == "2");
    REQUIRE(model->data(model->index(2, 0), Qt::DisplayRole).toString() == "3");
    REQUIRE(model->data(model->index(3, 0), Qt::DisplayRole).toString() == "4");
    REQUIRE(model->data(model->index(4, 0), Qt::DisplayRole).toString() == "5");
    REQUIRE(model->data(model->index(5, 0), Qt::DisplayRole).toString() == "6");
}

TEST_CASE("interrupt_parsing") {
    qapp();

    std::vector<std::string> trace;

    auto file = makeLogFile<TestLogFile>(simpleLog);

    QObject::connect(&file, &LogFile::stateChanged, [&] {
        trace.push_back(file.dbgStateName());
    });

    file.parse();

    waitFor([&] { return file.isState(gui::sm::ParsingState); });

    file.interrupt();

    file.parsingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::InterruptedState); });

    std::vector<std::string> expected {
        "gui::sm::ParsingState",
        "gui::sm::InterruptingState",
        "gui::sm::InterruptedState"
    };

    REQUIRE( trace == expected );
}

TEST_CASE("log_file_search_basic") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.search("4", false, false, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto model = file.logTableModel();
    auto searchModel = file.searchLogTableModel();
    auto hist = file.searchHist();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 2 );
    REQUIRE( hist != nullptr );
    REQUIRE( hist->get(0, 6) == 0 );
    REQUIRE( hist->get(1, 6) == 0 );
    REQUIRE( hist->get(2, 6) == 0 );
    REQUIRE( hist->get(3, 6) == 1 );
    REQUIRE( hist->get(4, 6) == 0 );
    REQUIRE( hist->get(5, 6) == 1 );

    searchModel->setSelection(0, 0, 0);
    REQUIRE( model->getRowSelection()->first == 3 );

    file.setColumnFilter(2, {"INFO"});
    auto selection = model->getRowSelection();
    REQUIRE( selection->first == 2 );

    // 10 INFO CORE message 1
    // 15 INFO SUB message 2
    // 20 INFO SUB message 4

    file.setColumnFilter(2, {});
    searchModel->setSelection(0, 0, 0);
    REQUIRE( !model->getRowSelection().has_value() );
}

TEST_CASE("log_file_search_case") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    // sensitive

    file.search("sub", false, true, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto model = file.logTableModel();
    auto searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 0 );

    // insensitive

    file.search("sub", false, false, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();
    searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 3 );
}

TEST_CASE("log_file_search_regex") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    // sensitive

    file.search("ERR|warn", true, true, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto model = file.logTableModel();
    auto searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 1 );

    // insensitive

    file.search("ERR|warn", true, false, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();
    searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 3 );

    // bad pattern

    file.search("[6-+", true, false, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();
    searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 0 );
}

TEST_CASE("log_file_search_message_only") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.search("4", false, true, true);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto model = file.logTableModel();
    auto searchModel = file.searchLogTableModel();

    REQUIRE( model->rowCount({}) == 6 );
    REQUIRE( searchModel->rowCount({}) == 1 );
    REQUIRE( searchModel->data(model->index(0, 1), Qt::DisplayRole).toString() == "20" );
}

TEST_CASE("column_max_width_should_be_set_after_file_has_been_indexed") {
    qapp();

    auto file = makeLogFile<TestLogFile>(simpleLog);
    file.parse();

    waitFor([&] { return file.isState(gui::sm::ParsingState); });

    auto model = file.logTableModel();
    auto longestColumnIndex = [&] (int index) {
        auto role = (int)HeaderDataRole::LongestColumnIndex;
        return model->headerData(index, Qt::Horizontal, role).toInt();
    };

    REQUIRE( longestColumnIndex(0) == -1 );
    REQUIRE( longestColumnIndex(1) == -1 );
    REQUIRE( longestColumnIndex(2) == -1 );
    REQUIRE( longestColumnIndex(3) == -1 );
    REQUIRE( longestColumnIndex(4) == -1 );

    file.parsingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::IndexingState); });

    file.indexingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    REQUIRE( longestColumnIndex(0) == 5 );
    REQUIRE( longestColumnIndex(1) != -1 );
    REQUIRE( longestColumnIndex(2) != -1 );
    REQUIRE( longestColumnIndex(3) != -1 );
    REQUIRE( longestColumnIndex(4) != -1 );

    file.interrupt();

    waitFor([&] { return file.isState(gui::sm::InterruptedState); });
}

TEST_CASE("log_file_color") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    auto regularColor = QColor(Qt::black);
    auto warningColor = QColor(0x550000);
    auto errorColor = QColor(0xff0000);

    auto model = file.logTableModel();
    REQUIRE( model->data(model->index(0, 0), Qt::ForegroundRole).value<QColor>() == regularColor );
    REQUIRE( model->data(model->index(0, 1), Qt::ForegroundRole).value<QColor>() == regularColor );

    REQUIRE( model->data(model->index(1, 2), Qt::ForegroundRole).value<QColor>() == regularColor );
    REQUIRE( model->data(model->index(1, 3), Qt::ForegroundRole).value<QColor>() == regularColor );

    REQUIRE( model->data(model->index(2, 0), Qt::ForegroundRole).value<QColor>() == warningColor );
    REQUIRE( model->data(model->index(2, 4), Qt::ForegroundRole).value<QColor>() == warningColor );

    REQUIRE( model->data(model->index(3, 2), Qt::ForegroundRole).value<QColor>() == regularColor );
    REQUIRE( model->data(model->index(3, 3), Qt::ForegroundRole).value<QColor>() == regularColor );

    REQUIRE( model->data(model->index(4, 0), Qt::ForegroundRole).value<QColor>() == errorColor );
    REQUIRE( model->data(model->index(4, 4), Qt::ForegroundRole).value<QColor>() == errorColor );

    REQUIRE( model->data(model->index(5, 2), Qt::ForegroundRole).value<QColor>() == warningColor );
    REQUIRE( model->data(model->index(5, 3), Qt::ForegroundRole).value<QColor>() == warningColor );
}

TEST_CASE("log_file_get_autosize_attibute") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);
    auto model = file.logTableModel();

    REQUIRE(model->headerData(0, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool());
    REQUIRE(model->headerData(1, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool());
    REQUIRE(!model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool());
    REQUIRE(!model->headerData(3, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool());
    REQUIRE(!model->headerData(4, Qt::Horizontal, (int)HeaderDataRole::IsAutosize).toBool());
}

TEST_CASE("dump_statemachine", "[.]") {
    boost::sml::sm<gui::sm::StateMachine> sm;
    std::ofstream f("/tmp/uml.txt");
    f << gui::sm::dump(sm);
}

TEST_CASE("reload_from_complete_state") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();

    REQUIRE(model->data(model->index(0, 4), Qt::DisplayRole).toString() == "message 1");

    file.reload(std::make_shared<std::stringstream>(simpleLogAlt));

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();

    REQUIRE(model->data(model->index(0, 4), Qt::DisplayRole).toString() == "alt 1");
}

TEST_CASE("reload_from_complete_state_with_different_parser") {
    qapp();

    auto file = makeLogFile(threeColumnLog, g_threeColumnTestParserName);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();

    REQUIRE(model->data(model->index(0, 4), Qt::DisplayRole).toString() == "F1");

    auto twoColumnParser = createTestParser();
    file.reload(std::make_shared<std::stringstream>(threeColumnLog), twoColumnParser);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();

    REQUIRE(model->data(model->index(0, 4), Qt::DisplayRole).toString() == "F1 message 1");
}

TEST_CASE("reload_from_complete_state_adapt_filters") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.setColumnFilter(2, {"INFO", "ERR"});

    file.reload(std::make_shared<std::stringstream>(simpleLogAlt));

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    REQUIRE( file.getColumnFilter(2) == std::set<std::string>{"INFO"} );

    auto model = file.logTableModel();

    REQUIRE( model->rowCount({}) == 2 );
    REQUIRE( model->data(model->index(0, 4), Qt::DisplayRole).toString() == "alt 2" );
    REQUIRE( model->data(model->index(1, 4), Qt::DisplayRole).toString() == "alt 3" );
}

TEST_CASE("reload_from_complete_state_drop_filters_if_parser_changed") {
    qapp();

    auto file = makeLogFile(threeColumnLog, g_threeColumnTestParserName);
    waitParsingAndIndexing(file);

    file.setColumnFilter(2, {"INFO", "ERR"});

    auto twoColumnParser = createTestParser();
    file.reload(std::make_shared<std::stringstream>(threeColumnLog), twoColumnParser);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    REQUIRE( file.getColumnFilter(2) == std::set<std::string>{} );

    auto model = file.logTableModel();

    REQUIRE( model->rowCount({}) == 6 );
}

TEST_CASE("log_file_clear_filters") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.setColumnFilter(2, {"INFO", "ERR"});

    auto model = file.logTableModel();
    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == true);
    REQUIRE(model->rowCount({}) == 4);

    file.clearFilters();

    REQUIRE(model->headerData(2, Qt::Horizontal, (int)HeaderDataRole::IsFilterActive).toBool() == false);
    REQUIRE( model->rowCount({}) == 6 );
}

TEST_CASE("reload_from_complete_state_after_search") {
    qapp();

    auto file = makeLogFile<TestLogFile>(simpleLog);
    file.parse();
    waitFor([&] { return file.isState(gui::sm::ParsingState); });
    file.parsingTask->proceed();
    waitFor([&] { return file.isState(gui::sm::IndexingState); });
    file.indexingTask->proceed();
    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto model = file.logTableModel();

    REQUIRE( model->data(model->index(0, 4), Qt::DisplayRole).toString() == "message 1" );

    file.search("message 1", false, false, false);

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    auto searchModel = file.searchLogTableModel();
    REQUIRE( searchModel->rowCount({}) == 1 );
    REQUIRE( searchModel->data(model->index(0, 4), Qt::DisplayRole).toString() == "message 1" );

    file.reload(std::make_shared<std::stringstream>(simpleLogAlt));

    waitFor([&] { return file.isState(gui::sm::ParsingState); });
    file.parsingTask->proceed();
    waitFor([&] { return file.isState(gui::sm::IndexingState); });
    file.indexingTask->proceed();
    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    searchModel = file.searchLogTableModel();
    REQUIRE( searchModel == nullptr );

    file.parsingTask->proceed();
    file.indexingTask->proceed();

    waitFor([&] { return file.isState(gui::sm::CompleteState); });

    model = file.logTableModel();

    REQUIRE( model->data(model->index(0, 4), Qt::DisplayRole).toString() == "alt 1" );
}

TEST_CASE("log_file_copy_raw_lines") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.setColumnFilter(2, {"INFO"});

    auto model = file.logTableModel();

    std::vector<std::string> expected {
        "10 INFO CORE message 1",
        "15 INFO SUB message 2",
        "20 INFO SUB message 4",
    };

    std::vector<std::string> actual;

    model->copyRawLines(0, 3, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("log_file_copy_lines") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    file.setColumnFilter(2, {"INFO"});

    auto model = file.logTableModel();

    std::vector<std::string> expected {
        "#   Timestamp   Level   Component   Message",
        "---------------------------------------------",
        "1   10          INFO    CORE        message 1",
        "2   15          INFO    SUB         message 2",
        "4   20          INFO    SUB         message 4",
    };

    std::vector<std::string> actual;

    model->copyLines(0, 3, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("log_file_copy_lines_unicode") {
    qapp();

    auto file = makeLogFile(u8"10 ИНФО CORE юникод\n"_as_char);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();

    std::vector<std::string> expected {
        "#   Timestamp   Level   Component   Message",
        "-------------------------------------------",
        "1   10          ИНФО    CORE        юникод",
    };

    std::vector<std::string> actual;

    model->copyLines(0, 1, [&] (auto line) {
        actual.push_back(line);
    });

    REQUIRE( actual == expected );
}

TEST_CASE("log_file_filter_exclude_include_clear") {
    qapp();

    auto file = makeLogFile(simpleLog);
    waitParsingAndIndexing(file);

    auto model = file.logTableModel();
    auto getRows = [&] {
        std::vector<std::string> rows;
        auto count = model->rowCount({});
        for (int i = 0; i < count; ++i) {
            auto value = model->data(model->index(i, 1), Qt::DisplayRole).toString();
            rows.push_back(value.toStdString());
        }
        return rows;
    };

    SECTION("clear column without filter") {
        file.clearFilter(2);
        REQUIRE(model->rowCount({}) == 6);
    }

    SECTION("exclude from single column") {
        file.excludeValue(2, "INFO");
        REQUIRE( getRows() == std::vector<std::string>{"17", "30", "40"} );

        SECTION("exclude column with existing filter") {
            file.excludeValue(2, "WARN");
            REQUIRE( getRows() == std::vector<std::string>{"30"} );
        }

        SECTION("exclude second column") {
            file.excludeValue(3, "CORE");
            REQUIRE( getRows() == std::vector<std::string>{"40"} );

            SECTION("clear one column out of two") {
                file.clearFilter(2);
                REQUIRE( getRows() == std::vector<std::string>{"15", "20", "40"} );
            }
        }
    }

    SECTION("include only value") {
        file.includeOnlyValue(2, "INFO");
        REQUIRE( getRows() == std::vector<std::string>{"10", "15", "20"} );
    }

    SECTION("restore selection after include") {
        model->setSelection(3, 0, 0);
        auto selection = *model->getRowSelection();
        REQUIRE( selection.first == 3 );
        REQUIRE( selection.last == 3 );

        file.includeOnlyValue(2, "INFO");
        selection = *model->getRowSelection();
        REQUIRE( selection.first == 2 );
        REQUIRE( selection.last == 2 );
    }

    SECTION("clear selection after exclude") {
        model->setSelection(4, 0, 0);
        file.excludeValue(2, "ERR");
        REQUIRE( !model->getRowSelection().has_value() );
    }

    SECTION("clear selection after excluding last row") {
        model->setSelection(5, 0, 0);
        file.excludeValue(2, "WARN");
        REQUIRE( !model->getRowSelection().has_value() );
    }
}
