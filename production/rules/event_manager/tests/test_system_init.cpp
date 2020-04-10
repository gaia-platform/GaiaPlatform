/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"
#include "rules.hpp"
#include "gaia_system.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::common;

extern "C" void initialize_rules()
{
}

TEST(event_manager_system_init, system_not_initialized_error)
{
    rule_binding_t dont_care;
    subscription_list_t still_dont_care;
    field_list_t ignore;

    EXPECT_THROW(log_table_event(nullptr, 0, event_type_t::row_insert, event_mode_t::immediate),
        initialization_error);
    EXPECT_THROW(log_transaction_event(event_type_t::transaction_begin, event_mode_t::immediate),
        initialization_error);
    EXPECT_THROW(subscribe_field_rule(0, ignore, dont_care),
        initialization_error);
    EXPECT_THROW(subscribe_table_rule(0, event_type_t::row_insert, dont_care),
        initialization_error);        
    EXPECT_THROW(subscribe_transaction_rule(event_type_t::transaction_rollback, dont_care),
        initialization_error);
    EXPECT_THROW(unsubscribe_table_rule(0, event_type_t::row_insert, dont_care),
        initialization_error);
    EXPECT_THROW(unsubscribe_transaction_rule(event_type_t::transaction_commit, dont_care),
        initialization_error);
    EXPECT_THROW(unsubscribe_rules(),
        initialization_error);
    EXPECT_THROW(list_subscribed_rules(nullptr, nullptr, nullptr, still_dont_care),
        initialization_error);
}

void rule(const context_base_t*)
{
}

TEST(event_manager_system_init, system_initialized)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    field_list_t fields;

    gaia::system::initialize(true);

    gaia_base_t::begin_transaction();
    EXPECT_EQ(false, log_table_event(nullptr, 0, event_type_t::row_insert, event_mode_t::immediate));
    EXPECT_EQ(false, log_transaction_event(event_type_t::transaction_begin, event_mode_t::immediate));
    subscribe_field_rule(0, fields, binding);
    subscribe_table_rule(0, event_type_t::row_insert, binding);
    subscribe_transaction_rule(event_type_t::transaction_begin, binding);
    EXPECT_EQ(true, unsubscribe_transaction_rule(event_type_t::transaction_begin, binding));
    EXPECT_EQ(true, unsubscribe_table_rule(0, event_type_t::row_insert, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, subscriptions);
    gaia_base_t::rollback_transaction();
}
