/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia/direct_access/auto_transaction.hpp"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::direct_access;

class auto_transaction_test : public db_test_base_t
{
};

TEST_F(auto_transaction_test, throw_if_active)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_active());
    {
        // Should be a no-op since a transaction is already active.
        EXPECT_THROW(auto_transaction_t txn, transaction_in_progress);
    }
    EXPECT_EQ(true, is_transaction_active());
    rollback_transaction();
}

TEST_F(auto_transaction_test, rollback_on_destruction)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Starts transaction then rollback on scope exit.
        auto_transaction_t txn;
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, commit_no_rollback)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        // Start transaction then commit.
        auto_transaction_t txn(false);
        EXPECT_EQ(true, is_transaction_active());
        txn.commit();
        EXPECT_EQ(false, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, invalid_nested)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn;
        {
            // No-op begin and rollback since
            // this instance doesn't own the transaction.
            EXPECT_THROW(auto_transaction_t nested, transaction_in_progress);
        }
        EXPECT_EQ(true, is_transaction_active());
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, invalid_commit_twice)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_begin);
        txn.commit();
        EXPECT_THROW(txn.commit(), no_open_transaction);
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, invalid_commit_mixed)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
        EXPECT_THROW(txn.commit(), no_open_transaction);
    }
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, no_throw_on_destruction)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
    } // Transaction is not active so the destructor shouldn't attempt to rollback.
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, rollback_existing)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
        gaia::db::begin_transaction();
    } // Transaction is active so the destructor should rollback.
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_begin_true)
{
    static_assert(false == auto_transaction_t::no_auto_begin, "auto_transaction_t::no_auto_begin constant should be false");
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn;
        EXPECT_EQ(true, is_transaction_active());
        txn.commit();
        // We begin a new transaction after commit
        EXPECT_EQ(true, is_transaction_active());
    } // Rollback the auto-begin transaction here.
    EXPECT_EQ(false, is_transaction_active());
}

TEST_F(auto_transaction_test, auto_begin_false)
{
    EXPECT_EQ(false, is_transaction_active());
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_begin);
        EXPECT_EQ(true, is_transaction_active());
        txn.commit();
        EXPECT_EQ(false, is_transaction_active());
    } // Nothing to rollback here.
    EXPECT_EQ(false, is_transaction_active());
}
