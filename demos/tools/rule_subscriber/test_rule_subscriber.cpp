/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "test_rules.hpp"
#include "gaia_system.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

using namespace ruleset_1;

uint8_t g_tx_handler_called = 0;
event_type_t g_event_type = event_type_t::row_delete;
void check_tx_handler(event_type_t expected_event, uint8_t expected_call)
{
    EXPECT_EQ(expected_call, g_tx_handler_called);
    EXPECT_EQ(expected_event, g_event_type);
    g_tx_handler_called = 0;
}

/**
 rule-1: [AddrBook::Employee](update, insert);[AddrBook::Employee.last_name];[AddrBook::Employee.first_name];
 [](commit)
 */
void ruleset_1::ObjectRule_handler(const rule_context_t*)
{
    g_tx_handler_called++;
}

/**
 rule-2: [AddrBook::Employee.last_name]; [AddrBook::Employee.first_name]; [AddrBook::Employee.phone_number]
 */
//void ruleset_2::Field_handler(const rule_context_t*)
//{
//    int x = 5;
//    x = x*2;
//}

/**
 rule-3: [AddrBook::Employee](delete)
 */
void ruleset_3::Table_handler(const rule_context_t*)
{
    int x = 5;
    x = x*2;
}

/**
 rule-4: [](commit, rollback)
*/
void ruleset_3::TransactionRule_handler(const rule_context_t* context)
{
    g_tx_handler_called++;
    g_event_type = context->event_type;
    // transaction events have record or type
    EXPECT_EQ((uint64_t)0, context->record);
    EXPECT_EQ((uint64_t)0, context->gaia_type);
}


TEST(rule_subscriber, tx_events)
{
    gaia::system::initialize(true);
    
    EXPECT_THROW(subscribe_ruleset("bogus"), ruleset_not_found);
    EXPECT_THROW(unsubscribe_ruleset("bogus"), ruleset_not_found);

    // Create a gaia object to ensure that the hooks are coded correctly
    // and overridden correctly
    AddrBook::Employee_ptr e;

    gaia::db::begin_transaction();
    // We did not hook rollback so we expect the same
    // event type from the begin.
    gaia::db::rollback_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_rollback);

    gaia::db::begin_transaction();
    gaia::db::commit_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_commit);
}