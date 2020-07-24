/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
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
void initialize_rules() {}

void rule1(const rule_context_t*)
{
    rule_count++;
}

rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};

class gaia_system_test : public ::testing::Test {};

// Insert one row and check if trigger is invoked.
// Todo (msj) Add more comprehensive tests and remove reliance on sleep_for.
TEST_F(gaia_system_test, insert_row_trigger) {
    start_server();
    begin_session();

    EXPECT_EQ(rule_count, 0);
    gaia::rules::initialize_rules_engine();
    subscribe_rule(m_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);

    begin_transaction();
    auto w = Employee_writer();
    w.name_first = "name";
    w.insert_row();
    commit_transaction();

    // The event_trigger_threadpool will invoke the rules engine on a separate thread from the client thread.
    // Each thread in the pool will lazily initialize a server session thus the first execution will be slow.
    // which is why we have a dumb while loop for now.
    auto count = 0;
    while(rule_count != 1 && count < 10) {  
        count ++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }
    EXPECT_EQ(rule_count, 1);
    unsubscribe_rules();
    end_session();
    stop_server();
}
