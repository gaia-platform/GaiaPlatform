/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "event_manager_test_helpers.hpp"
#include "rules.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::rules;
using namespace std;

extern "C" void initialize_rules()
{
}

class component_init_test : public db_test_base_t
{
};

TEST_F(component_init_test, component_not_initialized_error)
{
    rule_binding_t dont_care;
    subscription_list_t still_dont_care;
    field_position_list_t ignore;

    EXPECT_THROW(subscribe_rule(0, event_type_t::row_update, ignore, dont_care),
                 initialization_error);
    EXPECT_THROW(unsubscribe_rule(0, event_type_t::row_update, ignore, dont_care),
                 initialization_error);
    EXPECT_THROW(unsubscribe_rules(),
                 initialization_error);
    EXPECT_THROW(list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, still_dont_care),
                 initialization_error);
}

void rule(const rule_context_t*)
{
}

TEST_F(component_init_test, component_initialized)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    field_position_list_t fields;
    const field_position_t position = 10;
    fields.emplace_back(position);

    // Custom init disables catalog checks.
    event_manager_settings_t settings;
    settings.enable_catalog_checks = false;
    gaia::rules::test::initialize_rules_engine(settings);

    const gaia_type_t gaia_type = 1000;

    subscribe_rule(gaia_type, event_type_t::row_update, fields, binding);
    EXPECT_EQ(true, unsubscribe_rule(gaia_type, event_type_t::row_update, fields, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);
}
