/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "gtest/gtest.h"

#include "gaia_system.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "gaia_catalog_internal.hpp"
#include "rules.hpp"
#include "gaia_addr_book.h"
#include "triggers.hpp"
#include "db_test_base.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::catalog;
using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::addr_book;

static atomic<uint32_t> s_rule_count;
static atomic<uint32_t> s_rule_per_commit_count;

void rule1(const rule_context_t *) {
    s_rule_per_commit_count++;
    s_rule_count++;
}

extern "C" void initialize_rules() {
}

void load_catalog()
{
    gaia::catalog::ddl::field_def_list_t fields;
    // Add dummy catalog types for all our types used in this test.
    for (gaia_type_t i = employee_t::s_gaia_type; i <= phone_t::s_gaia_type; i++) 
    {
        string table_name = "dummy" + std::to_string(i);
        if (i == employee_t::s_gaia_type) {
            gaia::catalog::ddl::field_def_list_t emp_fields;
            emp_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"name_first", data_type_t::e_string, 1}));
            auto table_id = gaia::catalog::create_table("employee", emp_fields);
            begin_transaction();
            {
                auto field_ids = list_fields(table_id);
                for (gaia_id_t field_id : field_ids)
                {
                    // Mark all fields as active so that we can bind to them
                    gaia_field_writer w = gaia_field_t::get(field_id).writer();
                    w.active = true;
                    w.update_row();
                }
            }
            commit_transaction();
        } else {
            gaia::catalog::create_table(table_name, fields);
        }
    }
}

class gaia_system_test : public db_test_base_t {
public:
    static void SetUpTestSuite()
    {
        db_test_base_t::reset_server();
        begin_session();
        
        // NOTE: To run this test manually, you need to set the env variable DDL_FILE to the location of addr_book.ddl.
        // Currently this is under production/schemas/test/addr_book.
        const char *ddl_file = getenv("DDL_FILE"); //"/home/ubuntu/GaiaPlatform/production/schemas/test/addr_book/addr_book.ddl";
        ASSERT_NE(ddl_file, nullptr);

        // NOTE: For the unit test setup, we need to init catalog and load test tables before rules engine starts.
        //       Otherwise, the event log activities will cause out of order test table IDs.
        // gaia::catalog::load_catalog(ddl_file);

        gaia::rules::initialize_rules_engine();
        // load_catalog();

        // Initialize rules after loading the catalog.
        rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_delete, empty_fields, m_rule1);

        field_position_list_t fields;
        // We only have 1 field in this test.
        fields.push_back(0);
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_update, fields, m_rule1);
    }

    static void TearDownTestSuite()
    {
        end_session();
    }

protected:
    void SetUp() override {
        s_rule_count = 0;
        s_rule_per_commit_count = 0;
    }

    void TearDown() override {}
};

// This method will perform multiple transactions on the current client thread.
// Each transaction performs 3 operations.
void perform_transactions(uint32_t count_transactions, uint32_t crud_operations_per_tx, bool new_thread) {
    if (new_thread) {
        begin_session();
    }

    for (uint32_t i = 0; i < count_transactions; i++) {
        s_rule_per_commit_count = 0;
        begin_transaction();
        // Insert row.
        employee_writer w;
        w.name_first = "name";
        gaia_id_t id = w.insert_row();
        auto e = employee_t::get(id);

        // Update row.
        w = e.writer();
        w.name_first = "updated_name";
        w.update_row();

        // Delete row.
        employee_t::delete_row(id);
        gaia::db::commit_transaction();

        // We should get crud_operations_per_tx per commit.  Wait for them.
        while (s_rule_per_commit_count < crud_operations_per_tx) {
            usleep(1);
        }
    }

    if (new_thread) {
        end_session();
    }
}

void validate_and_end_test(uint32_t count_tx, uint32_t crud_operations_per_tx, uint32_t count_threads) {
    EXPECT_EQ(s_rule_count, count_tx * crud_operations_per_tx * count_threads);
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
