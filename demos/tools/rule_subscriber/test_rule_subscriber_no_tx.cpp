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
 rule-1: [AddrBook::Employee](update, insert);[AddrBook::Employee.name_last](write);[AddrBook::Employee.name_first](read, write);
  */
void ruleset_1::ObjectRule_handler(const rule_context_t* context)
{
    // never called because we didn't setup events here in the generated
    // addr book employee
    g_handler_called++;
    g_event_type = context->event_type;
    EXPECT_STREQ("Employee.name_first", context->event_source.c_str());
}

TEST(rule_subscriber, no_tx_events)
{
    AddrBook::Employee * e = new AddrBook::Employee();
    gaia::system::initialize(true);

    e->set_name_first("dax");
    check_handler(event_type_t::field_write, 1);
    delete e;

    // no transaction events
    gaia::db::begin_transaction();
    // no handler, no call for begin transaction
    check_handler(event_type_t::field_write, 0);

    gaia::db::rollback_transaction();
    check_handler(event_type_t::field_write, 0);

    gaia::db::begin_transaction();
    gaia::db::commit_transaction();
    check_handler(event_type_t::field_write, 0);
}