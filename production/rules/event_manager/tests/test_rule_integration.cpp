/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include <unistd.h>

#include <thread>
#include <atomic>

#include "gtest/gtest.h"

#include "rules.hpp"
#include "gaia_system.hpp"
#include "gaia_addr_book_db.h"
#include "db_test_base.hpp"
#include "gaia_catalog.hpp"
#include <thread>
#include <atomic>
#include <map>

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;
using namespace gaia::addr_book_db;
using namespace gaia::catalog;

const char* c_name = "John";
const char* c_city = "Seattle";
const char* c_state = "WA";
const char* c_phone_number = "867-5309";
const char* c_phone_type = "satellite";

uint16_t c_phone_number_position = 0;
uint16_t c_phone_type_position = 1;
uint16_t c_phone_primary_position = 2;

atomic<int> g_wait_for_count;
bool g_is_initialized = false;

// When an employee is inserted insert an address.
void rule_insert_address(const rule_context_t* context)
{
    employee_t e = employee_t::get(context->record);
    EXPECT_EQ(employee_t::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);

    if (0 == strcmp(c_name, e.name_first()))
    {
        address_writer aw;
        aw.city = c_city;
        aw.insert_row();
    }
}

// When an address is inserted, update the zip code of the
// inserted address.
void rule_update_address(const rule_context_t* context)
{
    EXPECT_EQ(address_t::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);
    address_t a = address_t::get(context->record);
    address_writer aw = a.writer();
    aw.state = c_state;
    aw.update_row();
    // Explicitly commit the transaction that was started by the rules engine.
    // If we don't do this then the results wouldn't be immediately visible
    // to the test thread when we decrement our count and the test would fail.
    // This also tests that the rules scheduler does the right thing when the
    // rule author commits the transaction in a rule.
    context->transaction.commit();
    g_wait_for_count--;
}

void rule_update(const rule_context_t* context)
{
    employee_t e = employee_t::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_name, e.name_first());
    g_wait_for_count--;
}

void rule_field_phone_number(const rule_context_t* context)
{
    phone_t p = phone_t::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_phone_number, p.phone_number());
    g_wait_for_count--;
}

void rule_field_phone_type(const rule_context_t* context)
{
    phone_t p = phone_t::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ(c_phone_type, p.type());
    g_wait_for_count--;
}

void rule_delete(const rule_context_t* context)
{
    employee_t d = employee_t::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_delete);
    EXPECT_THROW(d.delete_row(), invalid_node_id);
    g_wait_for_count--;
}

void rule_sleep(const rule_context_t*)
{
    sleep(1);
    g_wait_for_count--;
}

extern "C"
void initialize_rules()
{
}


// Waits for the rules to be called by checking
// for the counter to reach 0.
class rule_monitor_t
{
public:
    rule_monitor_t(int count) { g_wait_for_count = count; }
    ~rule_monitor_t() { while (g_wait_for_count > 0) { usleep(1); } }
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
        rule_binding_t rule1{"ruleset", "rule_insert_address", rule_insert_address};
        rule_binding_t rule2{"ruleset", "rule_update_address", rule_update_address};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule1);
        subscribe_rule(address_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule2);
    }

    void subscribe_delete()
    {
        rule_binding_t rule{"ruleset", "rule_delete", rule_delete};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_delete, empty_fields, rule);
    }

    void subscribe_update()
    {
        rule_binding_t rule{"ruleset", "rule_update", rule_update};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_update, empty_fields, rule);
    }

    void subscribe_sleep()
    {
        rule_binding_t rule{"ruleset", "rule_sleep", rule_sleep};
        subscribe_rule(employee_t::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule);
    }

    // We have two rules:  rule_field_phone_number and rule_phone_type.
    // The former is fired when phone_number changes and the latter is
    // fired when the type changes.  Both will fire if the 'primary' field
    // is changed.  This tests the following cases:
    // One rule subscribing to multiple active fields
    // Multiple rules suscribing to multiple active fields.
    void subscribe_field(uint16_t field_position)
    {
        field_position_list_t fields;
        fields.push_back(field_position);
        fields.push_back(c_phone_primary_position);

        rule_binding_t binding{"ruleset", nullptr, nullptr};

        if (field_position == c_phone_number_position)
        {
            binding.rule_name = "rule_field_phone_number";
            binding.rule = rule_field_phone_number;
        }
        else if (field_position == c_phone_type_position)
        {
            binding.rule_name = "rule_field_phone_type";
            binding.rule = rule_field_phone_type;
        }

        subscribe_rule(phone_t::s_gaia_type, triggers::event_type_t::row_update, fields, binding);
    }

    void load_catalog()
    {
        ddl::field_def_list_t empty_fields;

        // Add dummy catalog types for all our types with just dummy fields
        for (gaia_type_t i = event_log_t::s_gaia_type; i < phone_t::s_gaia_type; i++)
        {
            string table_name = "dummy" + std::to_string(i);
            gaia::catalog::create_table(table_name, empty_fields);
        }

        load_phone_table();
    }

    void load_phone_table()
    {
        // Our test type for field bindings (must be the last table in the addr_book_db schema so that inserting fields
        // doesn't throw off the type ids for tables that come after it).
        // Field positions are 0, 1, 2 and we mark them all as active in the catalog so that we can bind to them.
        gaia::catalog::ddl::field_def_list_t phone_fields;
        phone_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"phone_number", data_type_t::e_string, 1}));
        phone_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"type", data_type_t::e_string, 1}));
        phone_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"primary", data_type_t::e_bool, 1}));

        gaia_id_t phone_table = gaia::catalog::create_table("phone_t", phone_fields);
        auto field_ids = list_fields(phone_table);

        begin_transaction();
        {
            for (gaia_id_t field_id : field_ids)
            {
                // Mark all fields as active so that we can bind to them
                gaia_field_writer w = gaia_field_t::get(field_id).writer();
                w.active = true;
                w.update_row();
            }
        }
        commit_transaction();
    }

protected:
    rule_integration_test() : db_test_base_t(true) {
    }

    void SetUp() override {
        if (!g_is_initialized) {
            db_test_base_t::SetUp();
            gaia::system::initialize();
            load_catalog();
            g_is_initialized = true;
        }
    }

    void TearDown() override {
        unsubscribe_rules();
        // Currently a no-op.
        db_test_base_t::TearDown();
    }
};

TEST_F(rule_integration_test, test_insert)
{
    subscribe_insert();
    {
        rule_monitor_t monitor(1);

        auto_transaction_t tx(false);
        employee_writer writer;
        writer.name_first = c_name;
        writer.insert_row();
        tx.commit();
    }

    // Make sure the address was added and updated by the
    // rule that was fired above.
    {
        auto_transaction_t tx(false);
        address_t a = address_t::get_first();
        EXPECT_STREQ(a.city(), c_city);
        EXPECT_STREQ(a.state(), c_state);
    }
}

TEST_F(rule_integration_test, test_delete)
{
    subscribe_delete();
    {
        rule_monitor_t monitor(1);

        auto_transaction_t tx(true);
        employee_writer writer;
        writer.name_first = c_name;
        employee_t e = employee_t::get(writer.insert_row());
        tx.commit();

        e.delete_row();
        tx.commit();
    }
}

TEST_F(rule_integration_test, test_update)
{
    subscribe_update();
    {
        rule_monitor_t monitor(1);
        auto_transaction_t tx(true);
            employee_writer writer;
            writer.name_first = "Ignore";
            employee_t e = employee_t::get(writer.insert_row());
        tx.commit();
            writer = e.writer();
            writer.name_first = c_name;
            writer.update_row();
        tx.commit();
    }
}

// Test single rule, single active field binding.
TEST_F(rule_integration_test, test_update_field)
{
    subscribe_field(c_phone_number_position);
    {
        rule_monitor_t monitor(1);
        auto_transaction_t tx(true);
            phone_writer writer;
            writer.phone_number = "111-1111";
            phone_t p = phone_t::get(writer.insert_row());
        tx.commit();
            writer = p.writer();
            writer.phone_number = c_phone_number;
            writer.update_row();
        tx.commit();
    }
}

// Test that a different rule gets fired for different fields.
TEST_F(rule_integration_test, test_update_field_multiple_rules)
{
    subscribe_field(c_phone_number_position);
    subscribe_field(c_phone_type_position);
    {
        rule_monitor_t monitor(2);
        auto_transaction_t tx(true);
        phone_writer writer;
        writer.phone_number = "111-1111";
        //writer.type = "home";
        phone_t p = phone_t::get(writer.insert_row());
        tx.commit();
        writer = p.writer();
        writer.phone_number = c_phone_number;
        writer.type = c_phone_type;
        writer.update_row();
        tx.commit();
    }
}

// Test that the same rule gets fired for different active fields.
TEST_F(rule_integration_test, test_update_field_single_rule)
{
    subscribe_field(c_phone_number_position);
    {
        gaia_id_t phone_id;
        auto_transaction_t tx;

        phone_writer writer;
        writer.phone_number = "111-1111";
        writer.primary = false;
        phone_id = writer.insert_row();
        tx.commit();

        {
            // Changing the phone number should fire a rule.
            rule_monitor_t monitor(1);
            phone_writer writer = phone_t::get(phone_id).writer();
            writer.phone_number = c_phone_number;
            writer.update_row();
            tx.commit();
        }

        {
            // Changing the primary field should fire the rule.
            rule_monitor_t monitor(1);
            phone_writer writer = phone_t::get(phone_id).writer();
            writer.primary = true;
            writer.update_row();
            tx.commit();
        }
    }
}

TEST_F(rule_integration_test, test_two_rules)
{
    subscribe_update();
    subscribe_delete();
    {
        rule_monitor_t monitor(2);
        gaia_id_t first;
        gaia_id_t second;

        auto_transaction_t tx(true);
        employee_writer writer;
        writer.name_first = "Ignore";
        first = writer.insert_row();
        writer.name_first = "Me Too";
        second = writer.insert_row();
        tx.commit();

        // Delete first row and update second.
        employee_t::delete_row(first);
        writer = employee_t::get(second).writer();
        writer.name_first = c_name;
        writer.update_row();
        tx.commit();
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
    const int num_inserts = thread::hardware_concurrency();
    subscribe_sleep();
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point end;
    {
        rule_monitor_t monitor(num_inserts);
        auto_transaction_t tx(false);
        for (int i = 0; i < num_inserts; i++)
        {
            employee_t::insert_row("John", "Jones", "111-11-1111", i, nullptr, nullptr);
        }
        start = std::chrono::high_resolution_clock::now();
        tx.commit();
    }
    end = std::chrono::high_resolution_clock::now();
    int64_t total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count();
    double total_seconds = total_time / (double)1e9;
    EXPECT_TRUE(total_seconds < 2.0);
}
