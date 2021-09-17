/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_test_base.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace gaia::db::triggers;

void check_all_event_types(gaia_type_t context_type, gaia_type_t test_type, last_operation_t* expected)
{
    gaia::direct_access::auto_transaction_t txn;

    rule_context_t context(txn, context_type, event_type_t::row_update, 0);

    // Test the insert/update events which map to last operation types.
    EXPECT_EQ(expected ? *expected : last_operation_t::row_update, context.last_operation(test_type));

    context.event_type = event_type_t::row_insert;
    EXPECT_EQ(expected ? *expected : last_operation_t::row_insert, context.last_operation(test_type));

    // TODO[GAIAPLAT-194]: Transaction events are out of scope for Q2

    // Test event types that are not table operations.
    // context.event_type = event_type_t::transaction_begin;
    // EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));

    // context.event_type = event_type_t::transaction_commit;
    // EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));

    // context.event_type = event_type_t::transaction_rollback;
    // EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));
}

const gaia_type_t c_gaia_type = 42;
class rule_context_test : public db_test_base_t
{
};

TEST_F(rule_context_test, last_operation_type_match)
{
    // If the context type matches the passed-in type then we should get the
    // last_operation_t value mapped to a table event type.
    check_all_event_types(c_gaia_type, c_gaia_type, nullptr);
}

TEST_F(rule_context_test, last_operation_type_mismatch)
{
    // If the context type does not match the passed-in type then the last operation
    // performed on this type should be 'none'.
    last_operation_t expected = last_operation_t::none;
    check_all_event_types(0, c_gaia_type, &expected);
}

// Sanity check on compilation for const rule_context_t
TEST_F(rule_context_test, last_operation_type_const)
{
    gaia::direct_access::auto_transaction_t txn(false);
    const gaia_id_t record = 33;

    const rule_context_t context(txn, c_gaia_type, event_type_t::row_update, record);
    EXPECT_EQ(last_operation_t::row_update, context.last_operation(c_gaia_type));
    EXPECT_NO_THROW(context.txn.commit());
}
