/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include any internal headers to insert that
// we don't have a dependency on any internal implementation.

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/db/events.hpp"
#include "gaia/logger.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_addr_book.h"

using namespace std;

using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::db::triggers;

bool g_initialize_rules_called = false;
std::atomic_bool g_rule_1_called = false;

// Smoke tests public APIs, to ensure they work as expected and the symbols
// are exported (https://github.com/gaia-platform/GaiaPlatform/pull/397);
class sdk_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Load the schema in the database in a "public way"
        system("../catalog/gaiac/gaiac addr_book.ddl");
        gaia::system::initialize("./gaia.conf", "./gaia_log.conf");
    }

    void TearDown() override
    {
        gaia::system::shutdown();
    }
};

extern "C" void initialize_rules()
{
    // Verify this initialize_rules() is called
    g_initialize_rules_called = true;
}

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

template <typename T_ex, typename... T_args>
void test_exception(T_args... args)
{
    bool thrown = false;
    try
    {
        throw T_ex(args...);
    }
    catch (const T_ex& ex)
    {
        thrown = true;
    }

    ASSERT_TRUE(thrown) << "An exception should have been thrown";
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

    EXPECT_TRUE(g_initialize_rules_called);

    subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, binding);
    {
        auto_transaction_t tx;
        employee_writer w;
        w.name_first = "Public";
        w.name_last = "Headers";
        w.insert_row();
        // [GAIAPLAT-1205]:  We now do not fire an event if
        // the anchor row has been deleted.
        // employee_t e = employee_t::get(id);
        // e.delete_row();
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
    EXPECT_FALSE(gaia::db::is_transaction_open());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_open());
    gaia::db::commit_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_open());

    gaia::db::begin_transaction();
    EXPECT_TRUE(gaia::db::is_transaction_open());
    gaia::db::rollback_transaction();
    EXPECT_FALSE(gaia::db::is_transaction_open());
}

// The tests below ensure that all public exceptions can
// be referenced without compile or link errors.

// Catalog exceptions.
TEST_F(sdk_test, catalog_exceptions)
{
    test_exception<gaia::catalog::forbidden_system_db_operation>();
    test_exception<gaia::catalog::db_already_exists>();
    test_exception<gaia::catalog::db_does_not_exist>();
    test_exception<gaia::catalog::table_already_exists>();
    test_exception<gaia::catalog::table_does_not_exist>();
    test_exception<gaia::catalog::duplicate_field>();
    test_exception<gaia::catalog::field_does_not_exist>();
    test_exception<gaia::catalog::max_reference_count_reached>();
    test_exception<gaia::catalog::referential_integrity_violation>();
    test_exception<gaia::catalog::relationship_already_exists>();
    test_exception<gaia::catalog::relationship_does_not_exist>();
    test_exception<gaia::catalog::no_cross_db_relationship>();
    test_exception<gaia::catalog::relationship_tables_do_not_match>();
    test_exception<gaia::catalog::many_to_many_not_supported>();
    test_exception<gaia::catalog::index_already_exists>();
    test_exception<gaia::catalog::index_does_not_exist>();
    test_exception<gaia::catalog::invalid_relationship_field>();
    test_exception<gaia::catalog::ambiguous_reference_definition>();
    test_exception<gaia::catalog::orphaned_reference_definition>();
    test_exception<gaia::catalog::invalid_create_list>();
}

// Common exceptions.
TEST_F(sdk_test, common_exceptions)
{
    test_exception<gaia::common::configuration_error>();
    test_exception<gaia::common::logging::logger_exception>();
}

// Database exceptions.
TEST_F(sdk_test, db_exceptions)
{
    test_exception<gaia::db::session_exists>();
    test_exception<gaia::db::no_open_session>();
    test_exception<gaia::db::transaction_in_progress>();
    test_exception<gaia::db::no_open_transaction>();
    test_exception<gaia::db::transaction_update_conflict>();
    test_exception<gaia::db::transaction_object_limit_exceeded>();
    test_exception<gaia::db::duplicate_object_id>();
    test_exception<gaia::db::out_of_memory>();
    test_exception<gaia::db::system_object_limit_exceeded>();
    test_exception<gaia::db::invalid_object_id>();
    test_exception<gaia::db::object_still_referenced>();
    test_exception<gaia::db::object_too_large>();
    test_exception<gaia::db::invalid_object_type>();
    test_exception<gaia::db::session_limit_exceeded>();
    test_exception<gaia::db::invalid_reference_offset>();
    test_exception<gaia::db::invalid_relationship_type>();
    test_exception<gaia::db::single_cardinality_violation>();
    test_exception<gaia::db::child_already_referenced>();
    test_exception<gaia::db::invalid_child_reference>();
    test_exception<gaia::db::memory_allocation_error>();
    test_exception<gaia::db::pre_commit_validation_failure>();
    test_exception<gaia::db::index::unique_constraint_violation>();
}

// Direct access exceptions.
TEST_F(sdk_test, direct_access_exceptions)
{
    test_exception<gaia::direct_access::invalid_object_state>;
}

// Rule exceptions.
TEST_F(sdk_test, rule_exceptions)
{
    test_exception<invalid_rule_binding>();
    test_exception<duplicate_rule>();
    test_exception<initialization_error>();
    test_exception<invalid_subscription>();
    test_exception<ruleset_not_found>("ruleset_404");
}
