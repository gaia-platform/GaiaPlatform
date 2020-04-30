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

    EXPECT_THROW(log_database_event(nullptr, event_type_t::row_insert, event_mode_t::immediate),
        initialization_error);
    EXPECT_THROW(log_field_event(nullptr, nullptr, event_type_t::field_write, event_mode_t::immediate),
        initialization_error);
    EXPECT_THROW(subscribe_field_rule(0, event_type_t::field_write, ignore, dont_care),
        initialization_error);
    EXPECT_THROW(subscribe_database_rule(0, event_type_t::row_insert, dont_care),
        initialization_error);        
    EXPECT_THROW(unsubscribe_field_rule(0, event_type_t::field_write, ignore, dont_care),
        initialization_error);
    EXPECT_THROW(unsubscribe_database_rule(0, event_type_t::row_insert, dont_care),
        initialization_error);
    EXPECT_THROW(unsubscribe_rules(),
        initialization_error);
    EXPECT_THROW(list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, still_dont_care),
        initialization_error);
}

void rule(const rule_context_t*)
{
}

class row_context_t : public gaia_base_t
{
public:
    row_context_t() : gaia_base_t("TestGaia") {}
    void reset(bool) override {}

    static gaia_type_t s_gaia_type;
    gaia_type_t gaia_type() override
    {
        return s_gaia_type;
    }
};
gaia_type_t row_context_t::s_gaia_type = 1;

TEST(event_manager_system_init, system_initialized)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    field_list_t fields;
    fields.insert("last_name");
    row_context_t row;

    gaia::system::initialize(true);

    begin_transaction();
    EXPECT_EQ(false, log_database_event(&row, event_type_t::row_insert, event_mode_t::immediate));
    EXPECT_EQ(false, log_field_event(&row, "last_name", event_type_t::field_write, event_mode_t::immediate));
    subscribe_field_rule(row_context_t::s_gaia_type, event_type_t::field_write, fields, binding);
    subscribe_database_rule(row_context_t::s_gaia_type, event_type_t::row_insert, binding);
    EXPECT_EQ(true, unsubscribe_database_rule(1, event_type_t::row_insert, binding));
    EXPECT_EQ(true, unsubscribe_field_rule(1, event_type_t::field_write, fields, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);
    rollback_transaction();
}
