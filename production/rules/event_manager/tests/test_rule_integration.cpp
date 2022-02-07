/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <thread>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_addr_book.h"
#include "schema_loader.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace gaia::addr_book;
using namespace gaia::catalog;
using namespace std::chrono;

using std::atomic;
using std::string;
using std::thread;

constexpr char c_name[] = "John";
constexpr char c_city[] = "Seattle";
constexpr char c_state[] = "WA";
constexpr char c_phone_number[] = "867-5309";
constexpr char c_phone_waynetypeype[] = "satellite";

constexpr uint16_t c_phone_number_position = 0;
constexpr uint16_t c_phone_waynetypeype_position = 1;
constexpr uint16_t c_phone_primary_position = 2;

atomic<int> g_wait_for_count;
atomic<int> g_num_conflicts;
bool g_manual_commit;

// When an employee is inserted insert an address.
void rule_insert(const rule_context_t* context)
{
    employee_waynetype e = employee_waynetype::get(context->record);
    EXPECT_EQ(employee_waynetype::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);
    g_wait_for_count--;
}

void rule_insert_address(const rule_context_t* context)
{
    employee_waynetype e = employee_waynetype::get(context->record);
    EXPECT_EQ(employee_waynetype::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);

    if (0 == strcmp(c_name, e.name_first()))
    {
        address_writer aw;
        aw.city = c_city;
        aw.insert_row();
    }
    g_wait_for_count--;
}

void rule_update_helper(const rule_context_t* context)
{
    EXPECT_EQ(address_waynetype::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);
    address_waynetype a = address_waynetype::get(context->record);
    address_writer aw = a.writer();
    aw.state = c_state;
    aw.update_row();
}

// When an address is inserted, update the zip code of the
// inserted address.
void rule_update_address_commit(const rule_context_t* context)
{
    rule_update_helper(context);
    // Explicitly commit the transaction that was started by the rules engine.
    // If we don't do this then the results wouldn't be immediately visible
    // to the test thread when we decrement our count and the test would fail.
    // This also tests that the rules scheduler does the right thing when the
    // rule author commits the transaction in a rule.
    context->txn.commit();
    g_wait_for_count--;
}

void rule_update_address(const rule_context_t* context)
{
    rule_update_helper(context);
    g_wait_for_count--;
}

void rule_update(const rule_context_t* context)
{
    employee_waynetype e = employee_waynetype::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_name, e.name_first());
    g_wait_for_count--;
}

void rule_field_phone_number(const rule_context_t* context)
{
    phone_waynetype p = phone_waynetype::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_phone_number, p.phone_number());
    g_wait_for_count--;
}

void rule_field_phone_waynetypeype(const rule_context_t* context)
{
    phone_waynetype p = phone_waynetype::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_phone_waynetypeype, p.type());
    g_wait_for_count--;
}

// TODO[GAIAPLAT-445]: When we determine how we want to surface the deleted
// row event then we can re-instate these tests
// void rule_delete(const rule_context_t* context)
// {
//     employee_waynetype d = employee_waynetype::get(context->record);
//     EXPECT_EQ(context->event_type, triggers::event_type_t::row_delete);
//     EXPECT_THROW(d.delete_row(), invalid_object_id);
//     g_wait_for_count--;
// }

void rule_sleep(const rule_context_t*)
{
    sleep(1);
    g_wait_for_count--;
}

void rule_conflict(const rule_context_t* context)
{
    {
        auto ew = employee_waynetype::get(context->record).writer();
        ew.name_first = "Success";
        ew.update_row();
    }

    if (g_num_conflicts > 0)
    {
        g_num_conflicts--;
        thread([&context] {
            begin_session();
            {
                auto_transaction_t txn(auto_transaction_t::no_auto_restart);
                auto ew = employee_waynetype::get(context->record).writer();
                ew.name_first = "Conflict";
                ew.update_row();
                txn.commit();
            }
            end_session(); })
            .join();
    }

    g_wait_for_count--;

    if (g_manual_commit)
    {
        context->txn.commit();
    }
}

// Waits for the rules to be called by checking
// for the counter to reach 0.
class rule_monitor_t
{
public:
    explicit rule_monitor_t(int count)
    {
        g_wait_for_count = count;
    }
    ~rule_monitor_t()
    {
        gaia::rules::test::wait_for_rules_to_complete();
        EXPECT_EQ(g_wait_for_count, 0);
    }
};

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class rule_integration_test : public db_test_base_t
{
public:
    void subscribe_insert()
    {
        rule_binding_t rule1{"ruleset", "rule_insert", rule_insert};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule1);
    }

    void subscribe_insert_chain(bool commit_in_rule)
    {
        rule_binding_t rule1{"ruleset", "rule_insert_address", rule_insert_address};
        gaia_rule_fn fn = commit_in_rule ? rule_update_address_commit : rule_update_address;
        rule_binding_t rule2{"ruleset", "rule_update_address", fn};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule1);
        subscribe_rule(address_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule2);
    }

    // TODO[GAIAPLAT-445] We don't expose deleted row events
    // void subscribe_delete()
    // {
    //     rule_binding_t rule{"ruleset", "rule_delete", rule_delete};
    //     subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_delete, empty_fields, rule);
    // }

    void subscribe_update()
    {
        rule_binding_t rule{"ruleset", "rule_update", rule_update};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_update, empty_fields, rule);
    }

    void subscribe_sleep()
    {
        rule_binding_t rule{"ruleset", "rule_sleep", rule_sleep};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule);
    }

    void subscribe_sleep_serial()
    {
        rule_binding_t rule{"ruleset", "rule_sleep", rule_sleep, 0, "Serial"};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule);
    }

    // We have two rules:  rule_field_phone_number and rule_phone_waynetypeype.
    // The former is fired when phone_number changes and the latter is
    // fired when the type changes.  Both will fire if the 'primary' field
    // is changed.  This tests the following cases:
    // One rule subscribing to multiple active fields
    // Multiple rules suscribing to multiple active fields.
    void subscribe_field(uint16_t field_position)
    {
        field_position_list_t fields;
        fields.emplace_back(field_position);
        fields.emplace_back(c_phone_primary_position);

        rule_binding_t binding{"ruleset", nullptr, nullptr};

        if (field_position == c_phone_number_position)
        {
            binding.rule_name = "rule_field_phone_number";
            binding.rule = rule_field_phone_number;
        }
        else if (field_position == c_phone_waynetypeype_position)
        {
            binding.rule_name = "rule_field_phone_waynetypeype";
            binding.rule = rule_field_phone_waynetypeype;
        }

        subscribe_rule(phone_waynetype::s_gaia_type, triggers::event_type_t::row_update, fields, binding);
    }

    void subscribe_conflict()
    {
        rule_binding_t binding{"ruleset", "rule_conflict", rule_conflict};
        subscribe_rule(employee_waynetype::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, binding);
    }

protected:
    static void SetUpTestSuite()
    {
        db_test_base_t::SetUpTestSuite();

        // Do this before resetting the server to initialize the logger.
        gaia_log::initialize("./gaia_log.conf");

        begin_session();

        // NOTE: For the unit test setup, we need to init catalog and load test tables before rules engine starts.
        // Otherwise, the event log activities will cause out of order test table IDs.
        schema_loader_t::instance().load_schema("addr_book.ddl");

        event_manager_settings_t settings;

        // NOTE: uncomment the next line to enable individual rule stats from the rules engine.
        // settings.enable_rule_stats = true;
        gaia::rules::test::initialize_rules_engine(settings);
    }

    static void TearDownTestSuite()
    {
        gaia::rules::shutdown_rules_engine();
        end_session();
        gaia_log::shutdown();
    }

    // Ensure SetUp and TearDown don't do anything.  When we run the test
    // directly, we only want SetUpTestSuite and TearDownTestSuite
    void SetUp() override
    {
    }

    void TearDown() override
    {
        unsubscribe_rules();
    }
};

TEST_F(rule_integration_test, test_insert)
{
    bool commit_in_rule = true;
    subscribe_insert_chain(commit_in_rule);
    {
        rule_monitor_t monitor(2);

        auto_transaction_t txn(false);
        employee_writer writer;
        writer.name_first = c_name;
        writer.insert_row();
        txn.commit();
    }

    // Make sure the address was added and updated by the
    // rule that was fired above.
    {
        auto_transaction_t txn(false);
        address_waynetype a = *(address_waynetype::list().begin());
        EXPECT_STREQ(a.city(), c_city);
        EXPECT_STREQ(a.state(), c_state);
    }
}

// TODO[GAIAPLAT-445] We don't expose deleted row events
// TEST_F(rule_integration_test, test_delete)
// {
//     subscribe_delete();
//     {
//         rule_monitor_t monitor(1);

//         auto_transaction_t txn(true);
//         employee_writer writer;
//         writer.name_first = c_name;
//         employee_waynetype e = employee_waynetype::get(writer.insert_row());
//         txn.commit();
//         e.delete_row();
//         txn.commit();
//     }
// }
TEST_F(rule_integration_test, test_update)
{
    subscribe_update();
    {
        rule_monitor_t monitor(1);
        auto_transaction_t txn(true);
        employee_writer writer;
        writer.name_first = "Ignore";
        employee_waynetype e = employee_waynetype::get(writer.insert_row());
        txn.commit();
        writer = e.writer();
        writer.name_first = c_name;
        writer.update_row();
        txn.commit();
    }
}

// This test verifies that a rule is not invoked if the anchor row
// is deleted before the rule can be called.
TEST_F(rule_integration_test, test_update_and_delete)
{
    subscribe_update();
    {
        // The update rule should not be fired because we delete the row.
        rule_monitor_t monitor(0);
        auto_transaction_t txn(true);
        employee_writer writer;
        writer.name_first = "Ignore";
        employee_waynetype e = employee_waynetype::get(writer.insert_row());
        txn.commit();
        writer = e.writer();
        writer.name_first = c_name;
        writer.update_row();
        e.delete_row();
        txn.commit();
    }
}

// Test single rule, single active field binding.
TEST_F(rule_integration_test, test_update_field)
{
    subscribe_field(c_phone_number_position);
    {
        rule_monitor_t monitor(1);
        auto_transaction_t txn(true);
        phone_writer writer;
        writer.phone_number = "111-1111";
        phone_waynetype p = phone_waynetype::get(writer.insert_row());
        txn.commit();
        writer = p.writer();
        writer.phone_number = c_phone_number;
        writer.update_row();
        txn.commit();
    }
}

// Test that a different rule gets fired for different fields.
TEST_F(rule_integration_test, test_update_field_multiple_rules)
{
    subscribe_field(c_phone_number_position);
    subscribe_field(c_phone_waynetypeype_position);
    {
        rule_monitor_t monitor(2);
        auto_transaction_t txn(true);
        phone_writer writer;
        writer.phone_number = "111-1111";
        // writer.type = "home";
        phone_waynetype p = phone_waynetype::get(writer.insert_row());
        txn.commit();
        writer = p.writer();
        writer.phone_number = c_phone_number;
        writer.type = c_phone_waynetypeype;
        writer.update_row();
        txn.commit();
    }
}

// Test that the same rule gets fired for different active fields.
TEST_F(rule_integration_test, test_update_field_single_rule)
{
    subscribe_field(c_phone_number_position);
    {
        gaia_id_t phone_id;
        auto_transaction_t txn;

        phone_writer writer;
        writer.phone_number = "111-1111";
        writer.primary = false;
        phone_id = writer.insert_row();
        txn.commit();

        {
            // Changing the phone number should fire a rule.
            rule_monitor_t monitor(1);
            phone_writer writer = phone_waynetype::get(phone_id).writer();
            writer.phone_number = c_phone_number;
            writer.update_row();
            txn.commit();
        }

        {
            // Changing the primary field should fire the rule.
            rule_monitor_t monitor(1);
            phone_writer writer = phone_waynetype::get(phone_id).writer();
            writer.primary = true;
            writer.update_row();
            txn.commit();
        }
    }
}

// https://gaiaplatform.atlassian.net/browse/GAIAPLAT-1781
TEST_F(rule_integration_test, test_two_rules)
{
    subscribe_update();
    subscribe_insert();
    {
        // Two inserts and one update
        rule_monitor_t monitor(3);
        gaia_id_t first;
        gaia_id_t second;

        auto_transaction_t txn(true);
        employee_writer writer;
        writer.name_first = "Marty";
        first = writer.insert_row();
        writer.name_first = "McFly";
        second = writer.insert_row();
        txn.commit();

        // Update second record.
        employee_waynetype::delete_row(first);
        writer = employee_waynetype::get(second).writer();
        writer.name_first = c_name;
        writer.update_row();
        txn.commit();
    }
}

// Invoke the sleep rule which sleeps for 1 second.  We will call it the number
// of times equal to the number of hardware threads the system the test is
// running on. If we are truly running parallel then our total run time should
// be roughly 1s.  We'll be conservative and just verify its less than 2s. If we
// are not running parallel then the time will be equal to the number of times
// we invoke the sleep function.
TEST_F(rule_integration_test, test_parallel)
{
    const size_t num_inserts = thread::hardware_concurrency();

    // Don't use the optional_timer_t here because we actually do
    // want to get the duration of the function as part of this test.
    gaia::common::timer_t timer;
    subscribe_sleep();

    int64_t total_time = timer.get_function_duration([&]() {
        {
            rule_monitor_t monitor(num_inserts);
            auto_transaction_t txn(false);
            for (size_t i = 0; i < num_inserts; i++)
            {
                employee_waynetype::insert_row("John", "Jones", "111-11-1111", i, nullptr, nullptr);
            }
            txn.commit();
        } });
    double total_seconds = gaia::common::timer_t::ns_to_s(total_time);
    EXPECT_TRUE(total_seconds < 2.0);
}

// Invoke the sleep rule which sleeps for 1 second.  This time, however,
// associate it with a serial stream.
TEST_F(rule_integration_test, test_serial)
{
    const size_t num_inserts = 5;

    // Don't use the optional_timer_t here because we actually do
    // want to get the duration of the function as part of this test.
    gaia::common::timer_t timer;
    subscribe_sleep_serial();

    int64_t total_time = timer.get_function_duration([&]() {
        rule_monitor_t monitor(num_inserts);
        auto_transaction_t txn(false);
        for (size_t i = 0; i < num_inserts; i++)
        {
            employee_waynetype::insert_row("John", "Jones", "111-11-1111", i, nullptr, nullptr);
        }
        txn.commit(); });
    double total_seconds = gaia::common::timer_t::ns_to_s(total_time);
    EXPECT_TRUE(total_seconds >= num_inserts);
}

TEST_F(rule_integration_test, test_shutdown_pending_rules)
{
    // Verify that we execute all the rules we expect on shutdown.
    const size_t num_rule_invocations = 5;
    g_wait_for_count = num_rule_invocations;
    gaia::rules::shutdown_rules_engine();

    event_manager_settings_t settings;
    settings.num_background_threads = 1;
    gaia::rules::test::initialize_rules_engine(settings);

    subscribe_update();
    {
        auto_transaction_t txn;
        employee_writer writer;
        writer.name_first = c_name;
        gaia_id_t id = writer.insert_row();
        txn.commit();

        writer = employee_waynetype::get(id).writer();
        for (size_t i = 0; i < num_rule_invocations; i++)
        {
            writer.hire_date = i;
            writer.update_row();
        }
        txn.commit();
    }
    gaia::rules::shutdown_rules_engine();
    EXPECT_EQ(g_wait_for_count, 0);
    gaia::rules::initialize_rules_engine();
}

TEST_F(rule_integration_test, test_shutdown_rule_chaining)
{
    // Verify that we execute all the rules we expect on shutdown.
    // The insert rule we are firing causes an update which then
    // fires the update rule.
    const size_t num_rule_invocations = 4;
    g_wait_for_count = num_rule_invocations;
    gaia::rules::shutdown_rules_engine();

    event_manager_settings_t settings;
    settings.num_background_threads = 1;
    gaia::rules::test::initialize_rules_engine(settings);

    bool do_not_commit_in_rule = false;
    subscribe_insert_chain(do_not_commit_in_rule);
    {
        auto_transaction_t txn;
        employee_writer writer;
        writer.name_first = c_name;
        writer.insert_row();
        txn.commit();

        writer.name_first = c_name;
        writer.insert_row();
        txn.commit();
    }
    gaia::rules::shutdown_rules_engine();
    EXPECT_EQ(g_wait_for_count, 0);
    gaia::rules::initialize_rules_engine();
}

TEST_F(rule_integration_test, test_reinit)
{
    // Should be okay to call shutdown twice.
    gaia::rules::shutdown_rules_engine();
    gaia::rules::shutdown_rules_engine();

    // Should be okay to call init twice.
    gaia::rules::initialize_rules_engine();
    gaia::rules::initialize_rules_engine();
}

TEST_F(rule_integration_test, test_retry)
{
    auto test_inner = [&](int num_conflicts, int max_retries, const char* expected_name) {
        event_manager_settings_t settings;
        settings.max_rule_retries = max_retries;
        gaia::rules::test::initialize_rules_engine(settings);

        subscribe_conflict();

        // We execute the rule twice for each test to ensure that subsequent rules also have the
        // expected retry behavior.
        std::vector<string> names{"FooName", "BarName"};
        std::vector<gaia_id_t> ids;
        for (const auto& name : names)
        {
            // First rule execution isn't a retry, thus the "+ 1".
            rule_monitor_t monitor(std::min(num_conflicts, max_retries) + 1);
            g_num_conflicts = num_conflicts;
            auto_transaction_t txn(auto_transaction_t::no_auto_restart);
            employee_writer writer;
            writer.name_first = name;
            ids.emplace_back(writer.insert_row());
            txn.commit();
        }
        // Shut down the rules engine to ensure the rule fires.
        gaia::rules::shutdown_rules_engine();

        auto_transaction_t txn(auto_transaction_t::no_auto_restart);
        ASSERT_EQ(ids.size(), 2);
        for (auto id : ids)
        {
            EXPECT_EQ(string(employee_waynetype::get(id).name_first()), expected_name);
        }
    };

    // Each iteration of test_inner will initialize the rules engine with a different value for
    // max_rule_retries.
    gaia::rules::shutdown_rules_engine();

    // We want to make sure that we generate update conflicts from within the rule invocation
    // and from the auto-commit after the rule succeeds, so we set a flag to indicate which way we
    // want the rule to commit.
    for (auto manual_commit : {false, true})
    {
        g_manual_commit = manual_commit;

        test_inner(0, 0, "Success");
        test_inner(0, 1, "Success");
        test_inner(1, 0, "Conflict");
        test_inner(1, 1, "Success");
        test_inner(4, 3, "Conflict");
        test_inner(3, 3, "Success");
    }

    // And reinitialize to provide harmony for other tests.
    gaia::rules::initialize_rules_engine();
}
