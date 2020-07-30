/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "auto_tx.hpp"
#include "gtest/gtest.h"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::rules;

extern "C" void initialize_rules()
{
}

class auto_tx_test : public db_test_base_t {
};

TEST_F(auto_tx_test, auto_tx_active_commit)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_tx_t tx;
        tx.commit();
    }
    EXPECT_EQ(true, is_transaction_active());
    rollback_transaction();
}

TEST_F(auto_tx_test, auto_tx_active_rollback)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_tx_t tx;
    }
    EXPECT_EQ(true, is_transaction_active());
    rollback_transaction();
}

TEST_F(auto_tx_test, auto_tx_inactive_rollback)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Starts transaction then rollback on scope exit.
        auto_tx_t tx;
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_tx_test, auto_tx_inactive_commit)
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
