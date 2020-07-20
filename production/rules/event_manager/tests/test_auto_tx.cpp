/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "auto_tx.hpp"
#include "db_test_helpers.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::rules;


/*
TEST(auto_tx_test, auto_tx_active_commit)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_tx_t tx;
        tx.commit();
    }
    EXPECT_EQ(true, is_transaction_active());
}

TEST(auto_tx_test, auto_tx_active_rollback)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_tx_t tx;
    }
    EXPECT_EQ(true, is_transaction_active());
}
*/

TEST(auto_tx_test, auto_tx_inactive_rollback)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Starts transaction then rollback on scope exit.
        auto_tx_t tx;
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST(auto_tx_test, auto_tx_inactive_commit)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Starts transaction then commit.
        auto_tx_t tx;
        EXPECT_EQ(true, is_transaction_active());
        tx.commit();
        EXPECT_EQ(false, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  start_server();
  begin_session();
  int ret_code = RUN_ALL_TESTS();
  end_session();
  stop_server();
  return ret_code;
}
