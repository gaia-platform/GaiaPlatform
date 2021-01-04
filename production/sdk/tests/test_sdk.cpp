/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia/db/catalog.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "gaia_addr_book.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

uint32_t g_initialize_rules_called = 0;

extern "C" void initialize_rules()
{
    // Verify this initialize_rules() is called
    g_initialize_rules_called = 1;
}

void rule(const rule_context_t* ctx)
{
    // Ensure the linker exports rule_context_t::last_operation.
    if (ctx->last_operation(gaia::addr_book::employee_t::s_gaia_type) == last_operation_t::row_insert)
    {
        ASSERT_EQ(ctx->gaia_type, gaia::addr_book::employee_t::s_gaia_type);
    }
}

TEST(sdk_test, app_check)
{

    rule_binding_t binding("ruleset", "rulename", rule);
    subscription_list_t subscriptions;
    gaia::system::initialize("./gaia.conf", "./gaia_log.conf");

    EXPECT_EQ(g_initialize_rules_called, 1);
    gaia_type_t type_id = gaia::addr_book::employee_t::s_gaia_type;

    // Force a s_gaia_type creation in the Catalog (assumes that the Catalog is empty and the
    // first created table will get ID 1). ATM we do not expose an API to load DDL data into the
    // Catalog.
    gaia::catalog::create_table("test_table", gaia::catalog::ddl::field_def_list_t());

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
