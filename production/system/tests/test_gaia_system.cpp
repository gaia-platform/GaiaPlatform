/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <unistd.h>

#include <chrono>

#include <atomic>
#include <iostream>
#include <thread>

#include "gtest/gtest.h"

#include "gaia/db/catalog.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/db/db_test_helpers.hpp"
#include "gaia_internal/db/triggers.hpp"

#include "gaia_addr_book.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::db::triggers;
using namespace gaia::catalog;
using namespace gaia::rules;
using namespace gaia::common;
using namespace gaia::addr_book;

static atomic<uint32_t> g_rule_count;
static atomic<uint32_t> g_rule_per_commit_count;

void rule1(const rule_context_t*)
{
    g_rule_per_commit_count++;
    g_rule_count++;
}

class gaia_system_test : public db_test_base_t
{
public:
    static void SetUpTestSuite()
    {
        reset_server();
        begin_session();

        // NOTE: To run this test manually, you need to set the env variable DDL_FILE to the location of addr_book.ddl.
        // Currently this is under production/schemas/test/addr_book.
        const char* ddl_file = getenv("DDL_FILE");
        ASSERT_NE(ddl_file, nullptr);

        // NOTE: For the unit test setup, we need to init catalog and load test tables before rules engine starts.
        //       Otherwise, the event log activities will cause out of order test table IDs.
        gaia::catalog::load_catalog(ddl_file);

        gaia::rules::initialize_rules_engine();

        // Initialize rules after loading the catalog.
        rule_binding_t m_rule1{"ruleset1_name", "rule1_name", rule1};
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_insert, empty_fields, m_rule1);
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_delete, empty_fields, m_rule1);

        field_position_list_t fields;
        // We only have 1 field in this test.
        fields.emplace_back(0);
        subscribe_rule(employee_t::s_gaia_type, event_type_t::row_update, fields, m_rule1);
    }

    static void TearDownTestSuite()
    {
        end_session();
    }

protected:
    void SetUp() override
    {
        g_rule_count = 0;
        g_rule_per_commit_count = 0;
    }

    void TearDown() override
    {
    }
};

// This method will perform multiple transactions on the current client thread.
// Each transaction performs 3 operations.
void perform_transactions(uint32_t count_transactions, uint32_t crud_operations_per_txn, bool new_thread)
{
    if (new_thread)
    {
        begin_session();
    }

    for (uint32_t i = 0; i < count_transactions; i++)
    {
        g_rule_per_commit_count = 0;
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

        // We should get crud_operations_per_txn per commit.  Wait for them.
        while (g_rule_per_commit_count < crud_operations_per_txn)
        {
            usleep(1);
        }
    }

    if (new_thread)
    {
        end_session();
    }
}

void validate_and_end_test(uint32_t count_txn, uint32_t crud_operations_per_txn, uint32_t count_threads)
{
    EXPECT_EQ(g_rule_count, count_txn * crud_operations_per_txn * count_threads);
}

TEST_F(gaia_system_test, single_threaded_transactions)
{
    uint32_t count_txn = 2;
    uint32_t crud_operations_per_txn = 3;
    perform_transactions(count_txn, crud_operations_per_txn, false);
    validate_and_end_test(count_txn, crud_operations_per_txn, 1);
}

TEST_F(gaia_system_test, multi_threaded_transactions)
{
    uint32_t count_txn_per_thread = 1;
    uint32_t crud_operations_per_txn = 3;
    const uint32_t count_threads = 10;
    for (uint32_t i = 0; i < count_threads; i++)
    {
        auto t = std::thread(perform_transactions, count_txn_per_thread, crud_operations_per_txn, true);
        t.join();
    }
    validate_and_end_test(count_txn_per_thread, crud_operations_per_txn, count_threads);
}
