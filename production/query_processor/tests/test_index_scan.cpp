/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include "gtest/gtest.h"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/db_test_helpers.hpp"

#include "gaia_index_sandbox.h"

using namespace gaia::direct_access;

constexpr size_t c_num_initial_rows = 20;

class test_index_scan : public gaia::db::db_catalog_test_base_t
{
public:
    test_index_scan()
        : db_catalog_test_base_t("index_sandbox.ddl"){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        auto_transaction_t txn;

        for (size_t i = 0; i < c_num_initial_rows; i++)
        {
            auto w = gaia::index_sandbox::sandbox_writer();
            w.str = ""; // SAME
            w.f = static_cast<float>(i); // ASC
            w.i = c_num_initial_rows - i; // DESC
            w.insert_row();
        }

        txn.commit();
    }
};

TEST_F(test_index_scan, verify_cardinality)
{
    auto_transaction_t txn;

    size_t count = 0;

    for (auto& obj : gaia::index_sandbox::sandbox_t::list())
    {
        count++;
    }

    EXPECT_TRUE(count == c_num_initial_rows);
}

TEST_F(test_index_scan, insert_and_delete_entries)
{
    auto_transaction_t txn;
    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "HELLO";
    w.f = 21.0;
    w.i = -1;
    txn.commit();
}
