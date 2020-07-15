/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "rules.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::rules;


void check_all_event_types(
    gaia_type_t context_type, 
    gaia_type_t test_type, 
    last_operation_t* expected)
{
    rule_binding_t binding;
    rule_context_t context(binding, context_type, event_type_t::row_delete, 0);

    // Test the insert/update/delete events which map to last operation types.
    EXPECT_EQ(expected ? *expected : last_operation_t::row_delete, 
        context.last_operation(test_type));

    context.event_type = event_type_t::row_update;
    EXPECT_EQ(expected ? *expected : last_operation_t::row_update, 
        context.last_operation(test_type));

    context.event_type = event_type_t::row_insert;
    EXPECT_EQ(expected ? *expected : last_operation_t::row_insert, 
        context.last_operation(test_type));

    // Test event types that are not table operations.
    context.event_type = event_type_t::transaction_begin;
    EXPECT_EQ(expected ? *expected : last_operation_t::none, 
        context.last_operation(test_type));
    
    context.event_type = event_type_t::transaction_commit;
    EXPECT_EQ(expected ? *expected : last_operation_t::none, 
        context.last_operation(test_type));

    context.event_type = event_type_t::transaction_rollback;
    EXPECT_EQ(expected ? *expected : last_operation_t::none, 
        context.last_operation(test_type));
}

TEST(rule_context_test, last_operation_type_match)
{
    // If the context type matches the passed in type then we should get the
    // last_operation_t value mapped to a table event type.
    check_all_event_types(42, 42, nullptr);
}

TEST(rule_context_test, last_operation_type_mismatch)
{
    // If the context type does not match the passed in type then the last operation
    // preformed on this type should be 'none'.
    last_operation_t expected = last_operation_t::none;
    check_all_event_types(0, 42, &expected);
}
