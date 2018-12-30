#include <catch2/catch.hpp>

#include "gui/FilterTableModel.h"

TEST_CASE("preserve_order") {
    std::vector<seer::ColumnIndexInfo> values = {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    auto model = new gui::FilterTableModel(values);
    REQUIRE( model->rowCount(QModelIndex()) == 4 );
    REQUIRE( model->data(model->index(0, 1), Qt::DisplayRole).toString() == "124" );
    REQUIRE( model->data(model->index(1, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model->data(model->index(2, 1), Qt::DisplayRole).toString() == "123" );
    REQUIRE( model->data(model->index(3, 1), Qt::DisplayRole).toString() == "bc23" );
}

TEST_CASE("filter") {
    std::vector<seer::ColumnIndexInfo> values = {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    auto model = new gui::FilterTableModel(values);

    model->search("bc");
    REQUIRE( model->rowCount(QModelIndex()) == 2 );
    REQUIRE( model->data(model->index(0, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model->data(model->index(1, 1), Qt::DisplayRole).toString() == "bc23" );
    auto checked = model->checkedValues();
    REQUIRE( checked.size() == 4 );

    model->selectFound();
    checked = model->checkedValues();
    REQUIRE( checked.size() == 2 );
    REQUIRE( (checked.find("abc") != end(checked)) );
    REQUIRE( (checked.find("bc23") != end(checked)) );

    REQUIRE( model->data(model->index(0, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model->data(model->index(1, 0), Qt::CheckStateRole) == Qt::Checked );

    model->selectNone();
    checked = model->checkedValues();
    REQUIRE( checked.size() == 0 );
    model->selectAll();
    checked = model->checkedValues();
    REQUIRE( checked.size() == 4 );

    model->search("");
    REQUIRE( model->rowCount(QModelIndex()) == 4 );
    REQUIRE( model->data(model->index(0, 1), Qt::DisplayRole).toString() == "124" );
    REQUIRE( model->data(model->index(1, 1), Qt::DisplayRole).toString() == "abc" );
    REQUIRE( model->data(model->index(2, 1), Qt::DisplayRole).toString() == "123" );
    REQUIRE( model->data(model->index(3, 1), Qt::DisplayRole).toString() == "bc23" );
}

TEST_CASE("check") {
    std::vector<seer::ColumnIndexInfo> values = {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    auto model = new gui::FilterTableModel(values);
    model->selectNone();
    model->search("2");
    model->setData(model->index(1, 0), true, Qt::CheckStateRole);
    model->setData(model->index(2, 0), true, Qt::CheckStateRole);
    model->search("");
    REQUIRE( model->data(model->index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model->data(model->index(1, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model->data(model->index(2, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model->data(model->index(3, 0), Qt::CheckStateRole) == Qt::Checked );
}

TEST_CASE("initial_checked_value") {
    std::vector<seer::ColumnIndexInfo> values = {
        {"124", false, 2}, {"abc", true, 10}, {"123", false, 1}, {"bc23", true, 7}};
    auto model = new gui::FilterTableModel(values);
    REQUIRE( model->data(model->index(0, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model->data(model->index(1, 0), Qt::CheckStateRole) == Qt::Checked );
    REQUIRE( model->data(model->index(2, 0), Qt::CheckStateRole) == Qt::Unchecked );
    REQUIRE( model->data(model->index(3, 0), Qt::CheckStateRole) == Qt::Checked );
}

TEST_CASE("get_counts") {
    std::vector<seer::ColumnIndexInfo> values = {
        {"124", true, 2}, {"abc", true, 10}, {"123", true, 1}, {"bc23", true, 7}};
    auto model = new gui::FilterTableModel(values);
    REQUIRE( model->rowCount(QModelIndex()) == 4 );
    REQUIRE( model->data(model->index(0, 2), Qt::DisplayRole).toString() == "2" );
    REQUIRE( model->data(model->index(1, 2), Qt::DisplayRole).toString() == "10" );
    REQUIRE( model->data(model->index(2, 2), Qt::DisplayRole).toString() == "1" );
    REQUIRE( model->data(model->index(3, 2), Qt::DisplayRole).toString() == "7" );
}
