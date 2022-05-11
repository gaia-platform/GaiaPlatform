/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "benchmark_test_base.hpp"
#include "gaia_perf_basic.h"
#include "test_perf.hpp"

using namespace gaia::perf_basic;
using namespace gaia::db;
using namespace gaia::benchmark;
using namespace std;

class benchmark_read_basic : public benchmark_test_base
{
public:
    benchmark_read_basic()
        : benchmark_test_base("schemas/perf_basic.ddl", c_persistence_disabled)
    {
    }
};

void clear_database()
{
    clear_table<simple_table_t>();
    clear_table<simple_table_2_t>();
    clear_table<simple_table_3_t>();
}

void insert_data()
{
    gaia::db::begin_transaction();

    for (size_t i = 0; i < c_num_records; i++)
    {
        if (i > 0 && i % c_max_insertion_single_txn == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
        }

        simple_table_t::insert_row(i);
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }
}

TEST_F(benchmark_read_basic, table_scan)
{
    insert_data();

    auto work = []() {
        gaia::db::begin_transaction();

        size_t i = 0;
        for ([[maybe_unused]] auto& record :
             simple_table_t::list())
        {
            i++;
        }

        gaia::db::commit_transaction();

        ASSERT_EQ(c_num_records, i);
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        work, clear_database, "simple_table_t::table_scan", clear_db_after_each_iteration);
}

TEST_F(benchmark_read_basic, table_scan_data_access)
{
    insert_data();

    auto work = []() {
        gaia::db::begin_transaction();

        size_t i = 0;
        for (auto& record : simple_table_t::list())
        {
            (void)record.uint64_field();
            (void)record.gaia_id();
            i++;
        }

        gaia::db::commit_transaction();

        ASSERT_EQ(c_num_records, i);
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        work, clear_database, "simple_table_t::table_scan_data_access", clear_db_after_each_iteration);
}
