////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

// Do not include any internal headers to ensure that
// the code included in this file doesn't have a dependency
// on non-public APIs.
#include "test_sdk_base.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

std::atomic_bool g_rule_1_called = false;

extern "C" void initialize_rules()
{
    // Verify this initialize_rules() is called
    g_initialize_rules_called = true;
}

class sdk__test : public sdk_base
{
};

void rule_1(const rule_context_t* ctx)
{
    // Ensure the linker exports rule_context_t::last_operation.
    if (ctx->last_operation(gaia::addr_book::employee_t::s_gaia_type) == last_operation_t::row_insert)
    {
        ASSERT_EQ(ctx->gaia_type, gaia::addr_book::employee_t::s_gaia_type);
        g_rule_1_called = true;
    }
}

// Note that we have an internal wait_for_rule function
// but since it is not public, we can't use it here.
// Wait for a rule to be executed for up to 1 second.
void wait_for_rule(std::atomic_bool& rule_guard)
{
    for (int i = 0; i < 1000; ++i)
    {
        if (rule_guard.load())
        {
            return;
        }
        usleep(1000);
    }
}

TEST_F(sdk__test, rule_subscribe_unsubscribe)
{
    rule_binding_t binding("ruleset", "rulename", rule_1);

    EXPECT_TRUE(g_initialize_rules_called);

    subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding);
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Public";
        w.name_last = "Headers";
        w.insert_row();
        // We now do not fire an event if
        // the anchor row has been deleted.
        // employee_t e = employee_t::get(id);
        // e.delete_row();
        tx.commit();

        wait_for_rule(g_rule_1_called);
        EXPECT_TRUE(g_rule_1_called) << "rule_1 should have been called";
    }

    EXPECT_EQ(true, unsubscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding));
}

TEST_F(sdk__test, rule_list)
{
    rule_binding_t binding("ruleset", "rulename", rule_1);

    subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding);

    subscription_list_t subscriptions;

    gaia::rules::list_subscribed_rules(
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        subscriptions);

    ASSERT_EQ(1, subscriptions.size());

    auto& rule_subscription = *subscriptions.begin();

    ASSERT_STREQ("ruleset", rule_subscription->ruleset_name);
    ASSERT_STREQ("rulename", rule_subscription->rule_name);
    ASSERT_EQ(employee_t::s_gaia_type, rule_subscription->gaia_type);
    ASSERT_EQ(event_type_t::row_insert, rule_subscription->event_type);
}

// Rule exceptions.
TEST_F(sdk__test, rule_exceptions)
{
    test_exception<invalid_rule_binding>();
    test_exception<duplicate_rule>();
    test_exception<initialization_error>();
    test_exception<invalid_subscription>();
    test_exception<ruleset_not_found>("ruleset_404");
}
