/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "rules.hpp"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::rules;

void check_all_event_types(
    gaia_type_t context_type,
    gaia_type_t test_type,
    last_operation_t* expected)
{
    gaia::direct_access::auto_transaction_t txn;
    field_position_list_t fields;

    rule_context_t context(txn, context_type, event_type_t::row_delete, 0, fields);

    // Test the insert/update/delete events which map to last operation types.
    EXPECT_EQ(expected ? *expected : last_operation_t::row_delete,
        context.last_operation(test_type));

    context.event_type = event_type_t::row_update;
    EXPECT_EQ(expected ? *expected : last_operation_t::row_update,
        context.last_operation(test_type));

    context.event_type = event_type_t::row_insert;
    EXPECT_EQ(expected ? *expected : last_operation_t::row_insert,
        context.last_operation(test_type));

    // TODO[GAIAPLAT-194]: Transaction events are out of scope for Q2

    // Test event types that are not table operations.
    //context.event_type = event_type_t::transaction_begin;
    //EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));

    //context.event_type = event_type_t::transaction_commit;
    //EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));

    //context.event_type = event_type_t::transaction_rollback;
    //EXPECT_EQ(expected ? *expected : last_operation_t::none,
    //    context.last_operation(test_type));
}

class rule_context_test : public db_test_base_t
{
};

TEST_F(rule_context_test, last_operation_type_match)
{
    // If the context type matches the passed-in type then we should get the
    // last_operation_t value mapped to a table event type.
    check_all_event_types(42, 42, nullptr);
}

TEST_F(rule_context_test, last_operation_type_mismatch)
{
    // If the context type does not match the passed-in type then the last operation
    // performed on this type should be 'none'.
    last_operation_t expected = last_operation_t::none;
    check_all_event_types(0, 42, &expected);
}

// Sanity check on compilation for const rule_context_t
TEST_F(rule_context_test, last_operation_type_const)
{
    gaia::direct_access::auto_transaction_t txn(false);
    field_position_list_t fields;

    const rule_context_t context(txn, 42, event_type_t::row_update, 33, fields);
    EXPECT_EQ(last_operation_t::row_update, context.last_operation(42));
    EXPECT_NO_THROW(context.txn.commit());
}
