/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia_addr_book.h"
#include "gaia_system.hpp"
#include "rules.hpp"

using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;
using namespace gaia::addr_book;

extern "C" void initialize_rules()
{
}

void rule(const rule_context_t*)
{
}

TEST(sdk_test, app_check)
{
    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    gaia::system::initialize("./gaia_conf.toml");
    gaia_type_t type_id = gaia::addr_book::employee_t::s_gaia_type;

    subscribe_rule(type_id, event_type_t::row_insert, empty_fields, binding);
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Public";
        w.name_last = "Headers";
        gaia_id_t id = w.insert_row();
        employee_t e = employee_t::get(id);
        e.delete_row();
        // Don't change the state of the db at all (no commit).
    }

    EXPECT_EQ(true, unsubscribe_rule(type_id, event_type_t::row_insert, empty_fields, binding));
}
