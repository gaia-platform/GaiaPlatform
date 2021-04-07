/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "gtest/gtest.h"

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/events.hpp"
#include "gaia/exception.hpp"
#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_addr_book.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

uint32_t g_initialize_rules_called = 0;
std::atomic<uint32_t> g_rule_1_called = 0;

// Smoke tests public APIs, to ensure they work as expected and the symbols
// are exported (https://github.com/gaia-platform/GaiaPlatform/pull/397);
class sdk_test : public ::testing::Test
{
protected:
    void SetUp()
    {
        gaia::system::initialize("./gaia.conf", "./gaia_log.conf");

        // Force a s_gaia_type creation in the Catalog (assumes that the Catalog is empty and the
        // first created table will get ID 1 which is the ID of employee_t table).
        // ATM we do not expose an API to load DDL data into the Catalog.
        gaia::catalog::create_table("", "test_table", gaia::catalog::ddl::field_def_list_t(), false);
    }

    void TearDown()
    {
        gaia::system::shutdown();
    }
};

extern "C" void initialize_rules()
{
    // Verify this initialize_rules() is called
    g_initialize_rules_called = 1;
}

void rule_1(const rule_context_t* ctx)
{
    // Ensure the linker exports rule_context_t::last_operation.
    if (ctx->last_operation(gaia::addr_book::employee_t::s_gaia_type) == last_operation_t::row_insert)
    {
        ASSERT_EQ(ctx->gaia_type, gaia::addr_book::employee_t::s_gaia_type);
        g_rule_1_called = 1;
    }
}

// Wait for a rule to be executed for up to 1 second.
void wait_for_rule(std::atomic<uint32_t>& rule_guard)
{
    for (int i = 0; i < 1000; i++)
    {
        if (rule_guard.load() != 0)
        {
            return;
        }
        usleep(1000);
    }
}

TEST_F(sdk_test, auto_txn)
{
    auto_transaction_t tx;
    employee_writer w;
    w.name_first = "Public";
    w.name_last = "Headers";
    gaia_id_t id = w.insert_row();
    employee_t e = employee_t::get(id);
    e.delete_row();
    tx.commit();
}

TEST_F(sdk_test, rule_subscribe_unsubscribe)
{
    rule_binding_t binding("ruleset", "rulename", rule_1);

    EXPECT_EQ(g_initialize_rules_called, 1);

    subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding);
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Public";
        w.name_last = "Headers";
        gaia_id_t id = w.insert_row();
        employee_t e = employee_t::get(id);
        e.delete_row();
        tx.commit();

        wait_for_rule(g_rule_1_called);
        EXPECT_TRUE(g_rule_1_called) << "rule_1 should have been called";
    }

    EXPECT_EQ(true, unsubscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding));
}

TEST_F(sdk_test, rule_list)
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

template <typename T_ex, typename... T_args>
void test_exception(T_args... args)
{
    bool thrown = false;
    try
    {
        throw T_ex(args...);
    }
    catch (T_ex& ex)
    {
        thrown = true;
    }

    ASSERT_TRUE(thrown) << "AN exception should have ben thrown";
}

TEST_F(sdk_test, rule_exceptions)
{
    test_exception<invalid_rule_binding>();

    rule_binding_t binding("ruleset", "rulename", rule_1);
    test_exception<duplicate_rule>(binding, true);

    test_exception<initialization_error>();
    test_exception<invalid_subscription>(employee_t::s_gaia_type);
}

TEST_F(sdk_test, gaia_logger)
{
    static constexpr char c_const_char_msg[] = "const char star message";
    static const std::string c_string_msg = "string message";
    static constexpr int64_t c_int_msg = 1234;

    gaia_log::app().trace("trace");
    gaia_log::app().trace("trace const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().debug("debug");
    gaia_log::app().debug("debug const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().info("info");
    gaia_log::app().info("info const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().warn("warn");
    gaia_log::app().warn("warn const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().error("error");
    gaia_log::app().error("error const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
    gaia_log::app().critical("critical");
    gaia_log::app().critical("critical const char*: '{}', std::string: '{}', number: '{}'", c_const_char_msg, c_string_msg, c_int_msg);
}

TEST_F(sdk_test, transactions)
{
    EXPECT_FALSE(gaia::db::is_transaction_active());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_active());
    gaia::db::commit_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_active());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_active());
    gaia::db::rollback_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_active());
}

TEST_F(sdk_test, db_exceptions)
{
    test_exception<gaia::db::session_exists>();
    test_exception<gaia::db::no_active_session>();
    test_exception<gaia::db::transaction_in_progress>();
    test_exception<gaia::db::no_open_transaction>();
    test_exception<gaia::db::transaction_update_conflict>();
    test_exception<gaia::db::transaction_object_limit_exceeded>();
    test_exception<gaia::db::duplicate_id>(gaia::common::c_invalid_gaia_id);
    test_exception<gaia::db::oom>();
    test_exception<gaia::db::invalid_node_id>(gaia::common::c_invalid_gaia_id);
    test_exception<gaia::db::invalid_id_value>(gaia::common::c_invalid_gaia_id);
    test_exception<gaia::db::node_not_disconnected>(gaia::common::c_invalid_gaia_id, employee_t::s_gaia_type);
    test_exception<gaia::db::payload_size_too_large>(100, 100);
    test_exception<gaia::db::invalid_type>(employee_t::s_gaia_type);
}
