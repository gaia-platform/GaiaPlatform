/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <unistd.h>
#include "gtest/gtest.h"
#include "gaia_system.hpp"
#include "rules.hpp"
#include "db_test_helpers.hpp"
#include "../addr_book_gaia_generated.h"
#include "triggers.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::rules;
using namespace gaia::common;
using namespace AddrBook_;

static uint32_t rule_count = 0;

const gaia_type_t m_gaia_type = 1;
extern "C"
void initialize_rules() {

}

void rule1(const rule_context_t* context)
{
    rule_count++;
}

rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};

class gaia_system_test : public ::testing::Test {
public:
protected:

};

// Insert one row and check if trigger is invoked
// Todo (msj) Add more comprehensive tests and remove reliance on sleep.
TEST_F(gaia_system_test, get_field) {
    start_server();
    begin_session();

    EXPECT_EQ(rule_count, 0);
    gaia::rules::initialize_rules_engine();
    subscribe_rule(m_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);

    begin_transaction();
    auto w = Employee_writer();
    w.name_first = "name";
    gaia_id_t id = w.insert_row();
    commit_transaction();

    // The event_trigger_threadpool will invoke the rule engine on a separate thread from the client thread.
    // Changes won't get invoked instantly; which is why we have a dumb while loop for now.
    // Each thread in the pool will lazily initialize a server session thus the first execution will be slow.
    // This test just checks functional correctness. 
    auto count = 0;
    while(rule_count != 1 && count < 10) {  
        count ++;
        this_thread::sleep_for(chrono::milliseconds(100) );
    }
    unsubscribe_rules();
    end_session();
    stop_server();
}
