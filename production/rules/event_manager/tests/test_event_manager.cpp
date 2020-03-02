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

// our row context
class TestGaia : public gaia_base
{
public:
    static uint64_t gaia_type;
    int32_t i = 20;
};

uint64_t TestGaia::gaia_type = 333;


void test_event_api();
void test_rules_api();

int main()
{
    test_event_api();
//    test_rules_api();

    cout << endl << c_all_tests_passed << endl;
}

void test_event_api()
{
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t api tests started ***" << endl;
    cout << c_debug_output_separator_line_end << endl;

    // table event
    TestGaia tg;
    retail_assert(false == log_table_event(&tg, event_type::col_change, event_mode::deferred),
        "ERROR: log_table_event API failed.");

    // transaction event
    retail_assert(false == log_transaction_event(event_type::transaction_commit, event_mode::deferred),
        "ERROR: log_transaction_event API failed.");

    cout << "PASSED: Event Manager events api tests passed" << endl;
    cout << endl << c_debug_output_separator_line_start << endl;
    cout << "*** event_manager_t basic api tests ended ***" << endl;
    cout << c_debug_output_separator_line_end << endl;
}
