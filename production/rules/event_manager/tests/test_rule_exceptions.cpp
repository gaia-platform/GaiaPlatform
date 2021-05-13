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

#include "gtest/gtest.h"

#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_test_base.hpp"

#include "event_manager_test_helpers.hpp"
#include "gaia_addr_book.h"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;
using namespace gaia::addr_book;
using namespace gaia::catalog;

constexpr char c_name[] = "John";
constexpr int c_num_retries = 3;

enum exception_type_t
{
    invalid_node,
    update_conflict,
    standard,
    non_standard,
    count
};

atomic<int> g_exception_counters[exception_type_t::count] = {};

namespace gaia
{
namespace rules
{

class my_std_exception : public std::exception
{
};

class my_non_std_exception
{
};

// This test exception handler will handle the types of exceptions enumerated
// in exception_type_t.  All other exceptions will terminate the program.
extern "C" void handle_rule_exception()
{
    // A transaction should not be active if we got here because the rules engine
    // should have aborted the transaction prior to getting in to the catch handler.
    EXPECT_FALSE(gaia::db::is_transaction_active());

    try
    {
        throw;
    }
    catch (const gaia::db::invalid_object_id&)
    {
        g_exception_counters[exception_type_t::invalid_node]++;
    }
    catch (const gaia::db::transaction_update_conflict&)
    {
        g_exception_counters[exception_type_t::update_conflict]++;
    }
    catch (const my_std_exception&)
    {
        g_exception_counters[exception_type_t::standard]++;
    }
    catch (const my_non_std_exception&)
    {
        g_exception_counters[exception_type_t::non_standard]++;
    }
}

} // namespace rules
} // namespace gaia

void rule_std_exception(const rule_context_t*)
{
    throw my_std_exception();
}

void rule_non_std_exception(const rule_context_t*)
{
    throw my_non_std_exception();
}

void rule_invalid_node_exception(const rule_context_t*)
{

    employee_t bad;
    // Accessing this object should throw an exception because the employee does not exist in the database.
    bad = bad.get_next();
}

void rule_conflict_exception(const rule_context_t* context)
{
    {
        auto ew = employee_t::get(context->record).writer();
        ew.name_first = "Success";
        ew.update_row();
    }

    thread([&context] {
        begin_session();
        {
            auto_transaction_t txn(auto_transaction_t::no_auto_begin);
            auto ew = employee_t::get(context->record).writer();
            ew.name_first = "Conflict";
            ew.update_row();
            txn.commit();
        }
        end_session();
    }).join();
}

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class rule_exception_test : public db_test_base_t
{
public:
    void subscribe_invalid_node()
    {
        rule_binding_t rule{"ruleset", "rule_invalid_node", rule_invalid_node_exception};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule);
    }

    void subscribe_conflict()
    {
        rule_binding_t binding{"ruleset", "rule_conflict", rule_conflict_exception};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, binding);
    }

    void subscribe_std_exception()
    {
        rule_binding_t binding{"ruleset", "rule_std", rule_std_exception};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, binding);
    }

    void subscribe_non_std_exception()
    {
        rule_binding_t binding{"ruleset", "rule_non_std", rule_non_std_exception};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, binding);
    }

    void init_exception_counters()
    {
        for (auto& counter : g_exception_counters)
        {
            counter = 0;
        }
    }

    void verify_exception_counters(const exception_type_t counter, int value)
    {
        for (int i = 0; i < exception_type_t::count; i++)
        {
            if (i == counter)
            {
                EXPECT_EQ(g_exception_counters[i], value);
            }
            else
            {
                EXPECT_EQ(g_exception_counters[i], 0);
            }
        }
    }

    // All rules are triggered by an insert into the employees table.
    void trigger_rule()
    {
        auto_transaction_t txn(auto_transaction_t::no_auto_begin);
        employee_writer writer;
        writer.name_first = c_name;
        writer.insert_row();
        txn.commit();
    }

protected:
    static void SetUpTestSuite()
    {
        // Do this before resetting the server to initialize the logger.
        gaia_log::initialize("./gaia_log.conf");

        // NOTE: to run this test manually, you need to set the env variable DDL_FILE
        // to the location of addr_book.ddl.  Currently this is under production/schemas/test/addr_book.
        reset_server();
        const char* ddl_file = getenv("DDL_FILE");
        ASSERT_NE(ddl_file, nullptr);
        begin_session();

        // NOTE: For the unit test setup, we need to init catalog and load test tables before rules engine starts.
        // Otherwise, the event log activities will cause out of order test table IDs.
        load_catalog(ddl_file);

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

// Ensures std exceeptions are caught by the rules engine and
// propogated to the exception handler.
TEST_F(rule_exception_test, test_std_exception)
{
    init_exception_counters();

    subscribe_std_exception();
    trigger_rule();
    gaia::rules::shutdown_rules_engine();
    verify_exception_counters(exception_type_t::standard, 1);

    gaia::rules::initialize_rules_engine();
}

// Ensures exceptions that don't inherit from std::exception
// are caught by the rules engine and propogated to the exception
TEST_F(rule_exception_test, test_non_std_exception)
{
    init_exception_counters();

    subscribe_non_std_exception();
    trigger_rule();
    gaia::rules::shutdown_rules_engine();
    verify_exception_counters(exception_type_t::non_standard, 1);

    gaia::rules::initialize_rules_engine();
}

// Ensures the exception is caught by the rules engine and
// doesn't escape to the test process.
TEST_F(rule_exception_test, test_invalid_node_exception)
{
    init_exception_counters();

    subscribe_invalid_node();
    trigger_rule();
    gaia::rules::shutdown_rules_engine();
    verify_exception_counters(exception_type_t::invalid_node, 1);

    gaia::rules::initialize_rules_engine();
}

// This test ensures that we only throw a single transaction
// update conflict exception even though the rule that causes
// the conflict exception gets invoked multiple times.
TEST_F(rule_exception_test, test_update_conflict_exception)
{
    init_exception_counters();

    event_manager_settings_t settings;
    settings.max_rule_retries = c_num_retries;
    gaia::rules::test::initialize_rules_engine(settings);

    subscribe_conflict();
    trigger_rule();
    gaia::rules::shutdown_rules_engine();
    verify_exception_counters(exception_type_t::update_conflict, 1);

    gaia::rules::initialize_rules_engine();
}
