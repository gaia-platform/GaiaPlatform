/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "test_rules2.hpp"
#include "gaia_system.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

using namespace ruleset_1;

void ruleset_1::ObjectRule_handler(const rule_context_t*)
{
    int x =5;
    x=x*2;
}

/**
 rule-2: [AddrBook::Employee.last_name](write); [AddrBook::Employee.first_name](write); [AddrBook::Employee.phone_number](read)
 */
void ruleset_1::Field_handler(const rule_context_t*)
{
    int x =5;
    x=x*2;
}

/**
 rule-3: [AddrBook::Employee](delete)
 */
void ruleset_1::Table_handler(const rule_context_t*)
{
    int x =5;
    x=x*2;
}

/**
 rule-4: [](begin, commit, rollback)
*/

bool g_tx_handler_called = false;
event_type_t g_event_type = event_type_t::row_delete;

void ruleset_1::TransactionRule_handler(const rule_context_t* context)
{
    g_tx_handler_called = true;
    g_event_type = context->event_type;
    // transaction events have no context
    EXPECT_EQ(nullptr, context->event_context);
    EXPECT_EQ((uint64_t)0, context->gaia_type);
    EXPECT_STREQ("", context->event_source.c_str());
}


TEST(rule_subscriber, tx_events)
{
    AddrBook::Employee e;
    rule_binding_t binding("ruleset", "rulename", TransactionRule_handler);
    gaia::system::initialize(true);
    // create a gaia object to ensure that the hooks are coded correctly
    

    gaia::db::begin_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_begin);

    // We did not hook rollback so we expect the same
    // event type from the begin.
    gaia::db::rollback_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_begin);

    gaia::db::begin_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_begin);

    gaia::db::commit_transaction();
    EXPECT_EQ(g_event_type, event_type_t::transaction_commit);
}