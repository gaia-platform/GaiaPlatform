/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/db_test_helpers.hpp"

#include "gaia_index_sandbox.h"

using namespace gaia::db;
using namespace gaia::direct_access;

constexpr size_t c_num_initial_rows = 20;

class test_index_recovery : public gaia::db::db_catalog_test_base_t
{
public:
    test_index_recovery()
        : db_catalog_test_base_t("index_sandbox.ddl", true){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        begin_session();

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

TEST_F(test_index_recovery, recovery_test)
{
    end_session();
    reset_server();
    begin_session();
}
