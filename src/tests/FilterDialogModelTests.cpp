#include <catch2/catch.hpp>

#include "gui/FilterTableModel.h"

TEST_CASE("preserve_order") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    REQUIRE( model.rowCount({}) == 4 );
    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "124" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model.data(model.index(2, 1), Qt::DisplayRole).toString() == "123" );
    REQUIRE( model.data(model.index(3, 1), Qt::DisplayRole).toString() == "bc23" );
}

TEST_CASE("put_values_with_zero_counts_last") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 0}, {"123", true, 0}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    REQUIRE( model.rowCount({}) == 4 );
    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "124" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "bc23" );
    REQUIRE( model.data(model.index(2, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model.data(model.index(3, 1), Qt::DisplayRole).toString() == "123" );
}

TEST_CASE("filter") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });

    model.search("bc");
    REQUIRE( model.rowCount({}) == 2 );
    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "bc23" );
    auto checked = model.checkedValues();
    REQUIRE( checked.size() == 4 );

    model.selectFound();
    checked = model.checkedValues();
    REQUIRE( checked.size() == 2 );
    REQUIRE( (checked.find("abc") != end(checked)) );
    REQUIRE( (checked.find("bc23") != end(checked)) );

    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );

    model.selectNone();
    checked = model.checkedValues();
    REQUIRE( checked.size() == 0 );
    model.selectAll();
    checked = model.checkedValues();
    REQUIRE( checked.size() == 4 );

    model.search("");
    REQUIRE( model.rowCount({}) == 4 );
    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "124" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model.data(model.index(2, 1), Qt::DisplayRole).toString() == "123" );
    REQUIRE( model.data(model.index(3, 1), Qt::DisplayRole).toString() == "bc23" );
}

TEST_CASE("filter_case_insensitive") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"BC23", true, 7}};
    gui::FilterTableModel model([&] { return values; });

    model.search("bc");
    REQUIRE( model.rowCount({}) == 2 );
    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "BC23" );
    auto checked = model.checkedValues();
    REQUIRE( checked.size() == 4 );
}

TEST_CASE("check") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    model.selectNone();
    model.search("2");
    model.setData(model.index(1, 0), true, Qt::CheckStateRole);
    model.setData(model.index(2, 0), true, Qt::CheckStateRole);
    model.search("");
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Checked );
}

TEST_CASE("initial_checked_value") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", false, 2}, {"abc", true, 10}, {"123", false, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Checked );
}

TEST_CASE("get_counts") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    REQUIRE( model.rowCount({}) == 4 );
    REQUIRE( model.data(model.index(0, 2), Qt::DisplayRole).toString() == "2" );
    REQUIRE( model.data(model.index(1, 2), Qt::DisplayRole).toString() == "10" );
    REQUIRE( model.data(model.index(2, 2), Qt::DisplayRole).toString() == "1" );
    REQUIRE( model.data(model.index(3, 2), Qt::DisplayRole).toString() == "7" );
}

TEST_CASE("invert_check") {
    std::vector<seer::ColumnIndexInfo> values {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    gui::FilterTableModel model([&] { return values; });
    model.selectNone();

    model.invertSelection({0, 1});
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Unchecked );

    model.invertSelection({0, 1, 2, 3});
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Checked );

    model.invertSelection({0, 1, 2, 3});
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Unchecked );

    model.invertSelection({0, 1, 2, 3});
    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(3, 0), Qt::CheckStateRole) == Qt::Checked );
}

TEST_CASE("invert_check_filtered") {
    std::vector<seer::ColumnIndexInfo> values {
        {"ERROR", true, 2}, {"INFO", true, 10}};
    gui::FilterTableModel model([&] { return values; });
    model.selectNone();
    model.search("info");
    model.selectNone();

    model.invertSelection({0});
    REQUIRE( model.checkedValues() == std::set<std::string>{"INFO"} );
}

TEST_CASE("refresh_doesnt_trigger_model_reset_with_same_values") {
    std::vector<seer::ColumnIndexInfo> values {
        {"ERROR", true, 2}, {"INFO", true, 10}};
    gui::FilterTableModel model([&] { return values; });
    int resetCount = 0, searchResetCount = 0;
    QObject::connect(&model, &QAbstractTableModel::modelReset, [&] { resetCount++; });
    QObject::connect(&model, &gui::FilterTableModel::searchReset, [&] { searchResetCount++; });
    model.refresh();
    REQUIRE( model.checkedValues() == std::set<std::string>{"INFO", "ERROR"} );
    REQUIRE( resetCount == 0 );
    REQUIRE( searchResetCount == 0 );
}

TEST_CASE("refresh_reflects_new_value_state") {
    std::vector<seer::ColumnIndexInfo> values {
        {"ERROR", true, 2}, {"INFO", true, 10}};
    gui::FilterTableModel model([&] { return values; });
    int resetCount = 0, searchResetCount = 0;
    QObject::connect(&model, &QAbstractTableModel::modelReset, [&] { resetCount++; });
    QObject::connect(&model, &gui::FilterTableModel::searchReset, [&] { searchResetCount++; });

    values[0].checked = false;
    values[0].count = 15;
    model.refresh();

    REQUIRE( model.checkedValues() == std::set<std::string>{"INFO"} );
    REQUIRE( resetCount == 1 );
    REQUIRE( searchResetCount == 1 );

    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );

    REQUIRE( model.data(model.index(0, 2), Qt::DisplayRole).toString() == "15" );
    REQUIRE( model.data(model.index(1, 2), Qt::DisplayRole).toString() == "10" );
}

TEST_CASE("refresh_reflects_news_values") {
    std::vector<seer::ColumnIndexInfo> values {
        {"ERROR", true, 2}, {"INFO", true, 10}};
    gui::FilterTableModel model([&] { return values; });
    int resetCount = 0, searchResetCount = 0;
    QObject::connect(&model, &QAbstractTableModel::modelReset, [&] { resetCount++; });
    QObject::connect(&model, &gui::FilterTableModel::searchReset, [&] { searchResetCount++; });

    REQUIRE( model.rowCount({}) == 2 );

    values.push_back({"WARNING", true, 20});
    model.refresh();

    REQUIRE( model.checkedValues() == std::set<std::string>{"INFO", "ERROR", "WARNING"} );
    REQUIRE( resetCount == 1 );
    REQUIRE( searchResetCount == 1 );
    REQUIRE( model.rowCount({}) == 3 );

    REQUIRE( model.data(model.index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model.data(model.index(2, 0), Qt::CheckStateRole) == Qt::Checked );

    REQUIRE( model.data(model.index(0, 1), Qt::DisplayRole).toString() == "ERROR" );
    REQUIRE( model.data(model.index(1, 1), Qt::DisplayRole).toString() == "INFO" );
    REQUIRE( model.data(model.index(2, 1), Qt::DisplayRole).toString() == "WARNING" );

    REQUIRE( model.data(model.index(0, 2), Qt::DisplayRole).toString() == "2" );
    REQUIRE( model.data(model.index(1, 2), Qt::DisplayRole).toString() == "10" );
    REQUIRE( model.data(model.index(2, 2), Qt::DisplayRole).toString() == "20" );
}
