/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::common;
using namespace gaia::addr_book;

/**
 * This suite tests the array fields of Direct Access Classes (also known as DAC vectors).
 * These tests were originally in the DAC object test suite, but as more tests
 * were added, they were separated into this array field test suite.
 */
class direct_access__array_field__test : public db_catalog_test_base_t
{
protected:
    direct_access__array_field__test()
        : db_catalog_test_base_t("addr_book.ddl")
    {
    }
};

TEST_F(direct_access__array_field__test, insert)
{
    const char* customer_name = "Unibot";
    const int32_t q1_sales = 200;
    const int32_t q2_sales = 300;
    const int32_t q3_sales = 500;

    auto_transaction_t txn;
    const std::vector<int32_t> sales_by_quarter{q1_sales, q2_sales, q3_sales};
    gaia_id_t id = customer_t::insert_row(customer_name, sales_by_quarter);
    txn.commit();

    auto c = customer_t::get(id);
    EXPECT_TRUE(std::equal(sales_by_quarter.begin(), sales_by_quarter.end(), c.sales_by_quarter().data()));
}

TEST_F(direct_access__array_field__test, writer)
{
    const char* customer_name = "xorlab";
    const int32_t q1_sales = 100;
    const int32_t q2_sales = 200;
    const int32_t q3_sales = 300;

    auto_transaction_t txn;
    auto w = customer_writer();
    w.name = customer_name;
    w.sales_by_quarter = {q1_sales, q2_sales};
    gaia_id_t id = w.insert_row();
    txn.commit();

    auto c = customer_t::get(id);
    EXPECT_STREQ(c.name(), customer_name);
    EXPECT_EQ(c.sales_by_quarter()[0], q1_sales);
    EXPECT_EQ(c.sales_by_quarter()[1], q2_sales);

    w = c.writer();
    w.sales_by_quarter.push_back(q3_sales);
    w.update_row();
    txn.commit();

    EXPECT_EQ(customer_t::get(id).sales_by_quarter()[2], q3_sales);
}

TEST_F(direct_access__array_field__test, empty)
{
    // If the array field is not optional and no value is given when inserting
    // the row, an empty vector will be inserted (by FlatBuffers). This test
    // verifies the behavior.
    const char* customer_name = "Test Customer";

    auto_transaction_t txn;
    auto w = customer_writer();
    w.name = customer_name;
    gaia_id_t id = w.insert_row();
    txn.commit();

    auto c = customer_t::get(id);
    EXPECT_STREQ(c.name(), customer_name);
    EXPECT_EQ(c.sales_by_quarter().size(), 0);
}

TEST_F(direct_access__array_field__test, iteration)
{
    auto_transaction_t txn;
    customer_t customer = customer_t::get(customer_t::insert_row("Unibot", {200, 300, 500}));
    txn.commit();

    std::vector<int32_t> sales;
    for (auto sale : customer.sales_by_quarter())
    {
        sales.push_back(sale);
    }
    EXPECT_EQ(sales.size(), customer.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), customer.sales_by_quarter().data()));

    sales.clear();
    for (auto sales_iter = customer.sales_by_quarter().begin(); sales_iter != customer.sales_by_quarter().end(); ++sales_iter)
    {
        sales.push_back(*sales_iter);
    }
    EXPECT_EQ(sales.size(), customer.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), customer.sales_by_quarter().data()));

    sales.clear();
    for (uint32_t i = 0; i < customer.sales_by_quarter().size(); ++i)
    {
        sales.push_back(customer.sales_by_quarter()[i]);
    }
    EXPECT_EQ(sales.size(), customer.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), customer.sales_by_quarter().data()));

    sales.clear();
    std::vector<int32_t> expected_sales = {500, 300, 200};
    for (auto sales_iter = customer.sales_by_quarter().rbegin(); sales_iter != customer.sales_by_quarter().rend(); ++sales_iter)
    {
        sales.push_back(*sales_iter);
    }
    EXPECT_EQ(sales.size(), customer.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), expected_sales.begin()));
}
