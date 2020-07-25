/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation.

#include "unistd.h"
#include <unordered_map>
#include "gtest/gtest.h"
#include "rules.hpp"
#include "gaia_system.hpp"
#include "addr_book_gaia_generated.h"
#include "db_test_helpers.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::rules;
using namespace std;
using namespace AddrBook;

/**
 * Table Rule functions.
 *
 * Make sure the rule operations are commutative since we don't guarantee
 * the order that rules are fired.  Table rules write to the passed in "row"
 * data.
 */

bool g_wait_for_rule = false;

// When an employee is inserted insert my address
void rule_insert_address(const rule_context_t* context)
{
    Employee e = Employee::get(context->record);
    EXPECT_EQ(Employee::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);

    if (0 == strcmp("Dax", e.name_first()))
    {
        Address_writer aw;
        aw.city = "Seattle";
        aw.insert_row();
    }
//    g_wait_for_rule = false;
}

// When an address is inserted, update the zip code 
// This rule is fired in response to an address being inserted
void rule_update_address(const rule_context_t* context)
{
    auto_transaction_t tx(false);
    EXPECT_EQ(Address::s_gaia_type, context->gaia_type);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_insert);
    Address a = Address::get(context->record);
    Address_writer aw = a.writer();
    aw.state = "WA";
    aw.update_row();
    // explicit commit the transaction that was started by the rules engine
    tx.commit();
    g_wait_for_rule = false;
}

void rule_update(const rule_context_t* context)
{
    // Ensure transaction goes out of scope before letting the main program
    // continues.
    Employee e = Employee::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_update);
    EXPECT_STREQ("John", e.name_first());
    g_wait_for_rule = false;
}

void rule_delete(const rule_context_t* context)
{
    Employee d = Employee::get(context->record);
    EXPECT_EQ(context->event_type, triggers::event_type_t::row_delete);
    EXPECT_THROW(d.delete_row(), invalid_node_id);
    g_wait_for_rule = false;
}

 extern "C"
 void initialize_rules()
 {
 }

class rule_monitor_t
{
public:
    rule_monitor_t() { g_wait_for_rule = true; }
    ~rule_monitor_t() { while (g_wait_for_rule) { usleep(1); } }
};

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class rule_integration_test : public ::testing::Test
{
public:
    void subscribe_insert()
    {
        rule_binding_t rule1{"ruleset", "rule_insert_address", rule_insert_address};
        rule_binding_t rule2{"ruleset", "rule_update_address", rule_update_address};
        subscribe_rule(Employee::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule1);
        subscribe_rule(Address::s_gaia_type, triggers::event_type_t::row_insert, empty_fields, rule2);
    }

    void subscribe_delete()
    {
        rule_binding_t _rule1{"ruleset", "rule_delete", rule_delete};
        subscribe_rule(Employee::s_gaia_type, triggers::event_type_t::row_delete, empty_fields, _rule1);
    }

    void subscribe_update()
    {
        rule_binding_t _rule1{"ruleset", "rule_update", rule_update};
        subscribe_rule(Employee::s_gaia_type, triggers::event_type_t::row_update, empty_fields, _rule1);
    }

protected:
    static void SetUpTestSuite() {
        start_server();
        gaia::system::initialize();
    }

    static void TearDownTestSuite() {
        end_session();
        stop_server();
    }

    void SetUp() override {
    }

    void TearDown() override {
        unsubscribe_rules();
        delete_employees();
    }

    void delete_employees()
    {
        auto_transaction_t tx(false);
        for (Employee e = Employee::get_first(); e ; e=e.get_first())
        {
            e.delete_row();
        }
        tx.commit();
    }

};

TEST_F(rule_integration_test, test_insert)
{
    {
        rule_monitor_t monitor;
        subscribe_insert();

        auto_transaction_t tx(false);
            Employee_writer writer;
            writer.name_first = "Dax";
            writer.insert_row();
        tx.commit();
    }

    // make sure the address was added and updated
    {
        auto_transaction_t tx(false);
        Address a = Address::get_first();
        EXPECT_STREQ(a.city(), "Seattle");
        EXPECT_STREQ(a.state(), "WA");
    }
}

TEST_F(rule_integration_test, test_delete)
{
    rule_monitor_t monitor;
    subscribe_delete();

    auto_transaction_t tx(true);
        Employee_writer writer;
        writer.name_first = "Dax";
        Employee e = Employee::get(writer.insert_row());
    tx.commit();
        e.delete_row();
    tx.commit();
}

TEST_F(rule_integration_test, test_update)
{
    rule_monitor_t monitor;
    subscribe_update();

    auto_transaction_t tx(true);
        Employee_writer writer;
        writer.name_first = "Dax";
        Employee e = Employee::get(writer.insert_row());
    tx.commit();
        writer = e.writer();
        writer.name_first = "John";
        writer.update_row();
    tx.commit();
}
