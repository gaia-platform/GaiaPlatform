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
#include <atomic>

using namespace std;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::rules;
using namespace gaia::common;
using namespace AddrBook_;

static atomic<uint32_t> rule_count;
static atomic<uint32_t> rule_per_commit_count;

const gaia_type_t m_gaia_type = 1;

void rule1(const rule_context_t*)
{
    rule_per_commit_count++;
    rule_count++;
}

extern "C"
void initialize_rules() {
       rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};
       subscribe_rule(m_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);
       subscribe_rule(m_gaia_type, event_type_t::row_delete, empty_fields, m_rule1);
       subscribe_rule(m_gaia_type, event_type_t::row_update, empty_fields, m_rule1);
}

class gaia_system_test : public ::testing::Test
{
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
        rule_count = 0;
        rule_per_commit_count = 0;
    }
};

// This method will perform multiple transactions on the current client thread.
// Each transaction performs 3 operations.
void perform_transactions(uint32_t count_transactions, uint32_t crud_operations_per_tx, bool new_thread) {
    if (new_thread) {
        begin_session();
    }

    for (uint32_t i = 0; i < count_transactions; i++) {
        rule_per_commit_count = 0;
        begin_transaction();
        // Insert row.
        auto w = Employee_writer();
        w.name_first = "name";
        gaia_id_t id = w.insert_row();
        auto e = Employee::get(id);

        // Update row.
        e.writer().name_first = "name2";
        e.writer().update_row();

        // Delete row.
        e.delete_row();
        commit_transaction();

        // We should get crud_operations_per_tx per commit.  Wait for them.
        while (rule_per_commit_count < crud_operations_per_tx)
        {
            usleep(1);
        }
    }

    if (new_thread) {
        end_session();
    }
}

void validate_and_end_test(uint32_t count_tx, uint32_t crud_operations_per_tx, uint32_t count_threads) {
    // Total wait time is 10 seconds
    // uint32_t wait_time_ms = 100;
    // uint32_t wait_loop_count = 10;
    // The event_trigger_threadpool will invoke the rules engine on a separate thread from the client thread.
    // Each thread in the pool will lazily initialize a server session thus the first execution will be slow.
    // which is why we have a dumb while loop for now.

    /*
    while(rule_count != count_tx * crud_operations_per_tx * count_threads && count < 100) {
        count ++;
        //std::this_thread::sleep_for(std::chrono::milliseconds(100) );
    }
    */
    EXPECT_EQ(rule_count, count_tx * crud_operations_per_tx * count_threads);
}

TEST_F(gaia_system_test, single_threaded_transactions) {
    uint32_t count_tx = 2;
    uint32_t crud_operations_per_tx = 3;
    perform_transactions(count_tx, crud_operations_per_tx, false);
    validate_and_end_test(count_tx, crud_operations_per_tx, 1);
}


TEST_F(gaia_system_test, multi_threaded_transactions) {
    uint32_t count_tx_per_thread = 1;
    uint32_t crud_operations_per_tx = 3;
    uint32_t count_threads = 10;
    for (uint32_t i = 0; i < count_threads; i++) {
        auto t = std::thread(perform_transactions, count_tx_per_thread, crud_operations_per_tx, true);
        t.join();
    }
    validate_and_end_test(count_tx_per_thread, crud_operations_per_tx, count_threads);
}
