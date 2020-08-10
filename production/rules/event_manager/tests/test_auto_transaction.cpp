/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "db_test_base.hpp"
#include "auto_transaction.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::direct_access;

extern "C" void initialize_rules()
{
}

class auto_transaction_test : public db_test_base_t {
};

TEST_F(auto_transaction_test, auto_transaction_active_commit)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_transaction_t tx;
        tx.commit();
    }
    EXPECT_EQ(true, is_transaction_active());
    rollback_transaction();
}

TEST_F(auto_transaction_test, auto_transaction_active_rollback)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        auto_transaction_t tx;
    }
    EXPECT_EQ(true, is_transaction_active());
    rollback_transaction();
}

TEST_F(auto_transaction_test, auto_transaction_inactive_rollback)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Starts transaction then rollback on scope exit.
        auto_transaction_t tx;
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_transaction_inactive_commit)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Start transaction then commit.
        auto_transaction_t tx(false);
        EXPECT_EQ(true, is_transaction_active());
        tx.commit();
        EXPECT_EQ(false, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_transaction_nested_rollback)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t tx;
        {
            // No-op begin and rollback since
            // this instance doesn't own the transaction.
            auto_transaction_t nested;
        }
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_transaction_nested_commit)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t tx;
        {
            // No-op begin and rollback since
            // this instance doesn't own the transaction.
            auto_transaction_t nested;
            nested.commit();
        }
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_begin_true)
{
    static_assert(false == auto_transaction_t::no_auto_begin, "auto_transaction_t::no_auto_begin constant should be false");
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t tx;
        EXPECT_EQ(true, is_transaction_active());
        tx.commit();
        // We begin a new transaction after commit
        EXPECT_EQ(true, is_transaction_active());
    }// Rollback the auto-begin transaction here.
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_begin_false)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t tx(false);
        EXPECT_EQ(true, is_transaction_active());
        tx.commit();
        EXPECT_EQ(false, is_transaction_active());
    }// Nothing to rollback here.
    EXPECT_EQ(false, is_transaction_active());
}
