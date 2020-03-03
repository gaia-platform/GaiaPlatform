/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

// do not include event_manager.hpp to ensure that
// we don't have a dependency on the internal implementation
#include "rules.hpp"
#include "constants.hpp"
#include "retail_assert.hpp"

using namespace std;
using namespace gaia::events;
using namespace gaia::rules;
using namespace gaia::api;
using namespace gaia::common;

// Our test row context.
class TestGaia : public gaia_base
{
public:
    static const gaia_type_t gaia_type = 333;
    int32_t i = 20;
};

// Our rule in Test_Set_1;
void rule_1_abcdefg(const context_base_t * context)
{
    const table_context_t * t = static_cast<const table_context_t *>(context);
    TestGaia * row = static_cast<TestGaia *>(t->row);
    retail_assert(20 == row->i, "Did not get correct row context!");
}

void test_events_api();
void test_rules_api();

int main()
{
    test_events_api();
    test_rules_api();

    cout << endl << c_all_tests_passed << endl;
}

void test_events_api()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t event API tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    // table event
    TestGaia tg;
    retail_assert(false == log_table_event(&tg, event_type::col_change, event_mode::deferred),
        "ERROR: log_table_event API failed.");

    // transaction event
    retail_assert(false == log_transaction_event(event_type::transaction_commit, event_mode::deferred),
        "ERROR: log_transaction_event API failed.");

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t event API tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}

void test_rules_api()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t rule API tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    // bind a rule to a column change event
    rule_binding_t rb;
    rb.ruleset_name = "Test_Set_1";
    rb.rule = rule_1_abcdefg;

    retail_assert(false == subscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rb),
        "ERROR:  subscribe_table_rule API failed.");

    
    retail_assert(false == subscribe_transaction_rule(event_type::transaction_commit, rb),
        "ERROR:  subscribe_table_rule API failed.");

    retail_assert(false == unsubscribe_table_rule(TestGaia::gaia_type, event_type::col_change, rb),
        "ERROR:  unsubscribe_table_rule API failed.");

    retail_assert(false == unsubscribe_transaction_rule(event_type::col_change, rb),
        "ERROR:  unsubscribe_transaction_rule API failed.");

    std::vector<const char *> rule_names;
    list_subscribed_rules(nullptr, nullptr, nullptr, rule_names);
    
    gaia_type_t t = TestGaia::gaia_type;
    event_type et = event_type::col_change;
    list_subscribed_rules(rb.ruleset_name, &t, &et, rule_names);

    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t rule API tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
