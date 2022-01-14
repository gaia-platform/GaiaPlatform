/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/gaia_relationships.hpp"

#include "gaia_addr_book.h"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;

using std::string;
using std::thread;
using std::to_string;

class gaia_vector_test : public db_catalog_test_base_t
{
protected:
    gaia_vector_test()
        : db_catalog_test_base_t(string("addr_book.ddl")){};
};

TEST_F(gaia_vector_test, iterator_test)
{
    auto_transaction_t txn;
    gaia_id_t id = customer_t::insert_row("Unibot", {200, 300, 500});
    txn.commit();

    auto c = customer_t::get(id);
    std::vector<int32_t> sales;
    for (auto s : c.sales_by_quarter())
    {
        sales.push_back(s);
    }
    EXPECT_EQ(sales.size(), c.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), c.sales_by_quarter().data()));

    sales.clear();
    for (auto s1 = c.sales_by_quarter().begin(); s1 != c.sales_by_quarter().end(); ++s1)
    {
        sales.push_back(*s1);
    }
    EXPECT_EQ(sales.size(), c.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), c.sales_by_quarter().data()));

    sales.clear();
    for (uint32_t idx = 0; idx < c.sales_by_quarter().size(); ++idx)
    {
        sales.push_back(c.sales_by_quarter()[idx]);
    }
    EXPECT_EQ(sales.size(), c.sales_by_quarter().size());
    EXPECT_TRUE(std::equal(sales.begin(), sales.end(), c.sales_by_quarter().data()));
}
