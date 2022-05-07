/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_basic.h"
#include "test_perf.hpp"

using namespace gaia::benchmark;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::perf_basic;
using namespace std;

class test_read_perf_basic : public gaia::db::db_catalog_test_base_t
{
public:
    test_read_perf_basic()
        : db_catalog_test_base_t("perf_basic.ddl", true, true, true)
    {
    }

    void TearDown() override
    {
        if (gaia::db::is_transaction_open())
        {
            gaia::db::rollback_transaction();
        }

        db_catalog_test_base_t::TearDown();
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

TEST_F(test_read_perf_basic, table_scan)
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

TEST_F(test_read_perf_basic, table_scan_data_access)
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

TEST_F(test_read_perf_basic, filter_no_match)
{
    insert_data();

    auto work = []() {
        gaia::db::begin_transaction();

        size_t i = 0;
        for ([[maybe_unused]] auto& record :
             simple_table_t::list().where(simple_table_t::expr::uint64_field > c_num_records))
        {
            i++;
        }

        gaia::db::commit_transaction();

        ASSERT_EQ(0, i);
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        work, clear_database, "simple_table_t::filter_no_match", clear_db_after_each_iteration);
}

TEST_F(test_read_perf_basic, filter_match)
{
    insert_data();

    auto work = []() {
        gaia::db::begin_transaction();

        size_t i = 0;
        for ([[maybe_unused]] auto& record :
             simple_table_t::list().where(simple_table_t::expr::uint64_field >= (c_num_records / 2)))
        {
            i++;
        }

        gaia::db::commit_transaction();

        ASSERT_EQ(c_num_records / 2, i);
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        work,
        clear_database,
        gaia_fmt::format("simple_table_t::filter_match {} matches", c_num_records / 2),
        clear_db_after_each_iteration);
}

TEST_F(test_read_perf_basic, table_scan_read_size)
{
    for (size_t num_reads : {10UL, 100UL, 1000UL, /*100000UL, 1000000UL, c_num_records*/})
    {
        insert_data();

        auto work = [num_reads]() {
            gaia::db::begin_transaction();

            size_t i = 0;
            for ([[maybe_unused]] auto& record :
                 simple_table_t::list())
            {
                if (++i == num_reads)
                {
                    break;
                }
            }

            gaia::db::commit_transaction();

            ASSERT_EQ(num_reads, i);
        };

        bool clear_db_after_each_iteration = false;
        run_performance_test(
            work,
            clear_database,
            gaia_fmt::format("simple_table_t::table_scan_size num_reads={}", num_reads),
            clear_db_after_each_iteration,
            num_reads < 1000 ? c_num_iterations * 10 : c_num_iterations,
            num_reads);
    }
}
