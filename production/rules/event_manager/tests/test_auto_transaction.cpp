////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <gtest/gtest.h>

#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/exceptions.hpp"

#include "gaia_internal/db/db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::direct_access;

class rules__auto_transaction__test : public db_test_base_t
{
};

TEST_F(rules__auto_transaction__test, throw_if_active)
{
    begin_transaction();
    EXPECT_EQ(true, is_transaction_open());
    {
        // Should be a no-op since a transaction is already active.
        EXPECT_THROW(auto_transaction_t txn, transaction_in_progress);
    }
    EXPECT_EQ(true, is_transaction_open());
    rollback_transaction();
}

TEST_F(rules__auto_transaction__test, rollback_on_destruction)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        // Starts transaction then rollback on scope exit.
        auto_transaction_t txn;
        EXPECT_EQ(true, is_transaction_open());
    }
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, commit_no_rollback)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        // Start transaction then commit.
        auto_transaction_t txn(false);
        EXPECT_EQ(true, is_transaction_open());
        txn.commit();
        EXPECT_EQ(false, is_transaction_open());
    }
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, invalid_nested)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn;
        {
            // No-op begin and rollback since
            // this instance doesn't own the transaction.
            EXPECT_THROW(auto_transaction_t nested, transaction_in_progress);
        }
        EXPECT_EQ(true, is_transaction_open());
    }
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, invalid_commit_twice)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_restart);
        txn.commit();
        EXPECT_THROW(txn.commit(), no_open_transaction);
    }
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, invalid_commit_mixed)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
        EXPECT_THROW(txn.commit(), no_open_transaction);
    }
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, no_throw_on_destruction)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
    } // Transaction is not active so the destructor shouldn't attempt to rollback.
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, rollback_existing)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn;
        gaia::db::commit_transaction();
        gaia::db::begin_transaction();
    } // Transaction is active so the destructor should rollback.
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, auto_begin_true)
{
    static_assert(false == auto_transaction_t::no_auto_restart, "auto_transaction_t::no_auto_restart constant should be false");
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn;
        EXPECT_EQ(true, is_transaction_open());
        txn.commit();
        // We begin a new transaction after commit
        EXPECT_EQ(true, is_transaction_open());
    } // Rollback the auto-begin transaction here.
    EXPECT_EQ(false, is_transaction_open());
}

TEST_F(rules__auto_transaction__test, auto_begin_false)
{
    EXPECT_EQ(false, is_transaction_open());
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_restart);
        EXPECT_EQ(true, is_transaction_open());
        txn.commit();
        EXPECT_EQ(false, is_transaction_open());
    } // Nothing to rollback here.
    EXPECT_EQ(false, is_transaction_open());
}
