/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "test_rules_no_tx.hpp"
#include "gaia_system.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

using namespace ruleset_1;

uint8_t g_handler_called = 0;
event_type_t g_event_type = event_type_t::row_delete;
void check_handler(event_type_t expected_event, uint8_t expected_call)
{
    EXPECT_EQ(expected_call, g_handler_called);
    EXPECT_EQ(expected_event, g_event_type);
    g_handler_called = 0;
}

/**
 rule-1: [AddrBook::Employee](update, insert);[AddrBook::Employee.name_last];[AddrBook::Employee.name_first];
  */
void ruleset_1::ObjectRule_handler(const rule_context_t* context)
{
    g_handler_called++;
    g_event_type = context->event_type;
}

TEST(rule_subscriber, no_tx_events)
{
    gaia::system::initialize(true); 

    // no transaction events
    gaia::db::begin_transaction();
    {
        AddrBook::Employee_writer w = AddrBook::Employee::writer();
        w->name_first = "dax";
        AddrBook::Employee::insert_row(w);
    }
    gaia::db::commit_transaction();
    check_handler(event_type_t::row_insert, 1);

    gaia::db::begin_transaction();
    gaia::db::rollback_transaction();
    check_handler(event_type_t::row_insert, 0);

    gaia::db::begin_transaction();
    check_handler(event_type_t::row_insert, 0);
    gaia::db::commit_transaction();

    // unsubscribe
    g_event_type = event_type_t::row_delete;
    unsubscribe_ruleset("ruleset_1");
    gaia::db::begin_transaction();
    AddrBook::Employee_ptr e = AddrBook::Employee::get_first();
    AddrBook::Employee_writer w = AddrBook::Employee::writer(e);
    w->web = "mygollum.com";
    AddrBook::Employee::update_row(w);
    gaia::db::commit_transaction();
    check_handler(event_type_t::row_delete, 0);

    // resubscribe should enable update event
    subscribe_ruleset("ruleset_1");
    gaia::db::begin_transaction();
    e = AddrBook::Employee::get_first();
    w = AddrBook::Employee::writer(e);
    w->web = "mygollum.com";
    AddrBook::Employee::update_row(w);
    gaia::db::commit_transaction();
    check_handler(event_type_t::row_update, 1);
}