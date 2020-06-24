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
using namespace gaia::direct;

extern "C" void initialize_rules()
{
}

TEST(event_manager_system_init, system_not_initialized_error)
{
    rule_binding_t dont_care;
    subscription_list_t still_dont_care;
    field_list_t ignore;

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
    fields.insert(5);
    row_context_t row;

    gaia::system::initialize(true);

    subscribe_rule(row_context_t::s_gaia_type, event_type_t::row_update, fields, binding);
    EXPECT_EQ(true, unsubscribe_rule(row_context_t::s_gaia_type, event_type_t::row_update, fields, binding));
    unsubscribe_rules();
    list_subscribed_rules(nullptr, nullptr, nullptr, nullptr, subscriptions);
}
