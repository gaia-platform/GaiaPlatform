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

class direct_access__perf_insert_basic__test : public gaia::db::db_catalog_test_base_t
{
public:
    direct_access__perf_insert_basic__test()
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

void insert_thread(size_t num_records, size_t txn_size = c_max_insertion_single_txn)
{
    gaia::db::begin_session();
    gaia::db::begin_transaction();

    for (size_t i = 0; i < num_records; i++)
    {
        if (i > 0 && i % txn_size == 0)
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

    gaia::db::end_session();
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_insert)
{
    auto insert = []() {
        bulk_insert(&simple_table_t::insert_row);
    };

    run_performance_test(
        insert, clear_database, "simple_table_t::insert_row");
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_writer)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_writer w;
            w.uint64_field = i;
            w.insert_row();
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_writer");
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_2)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_2_t::insert_row(i, "suppini", {1, 2, 3, 4, 5});
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_2_t::insert_row");
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_3)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_3_t::insert_row(
                i, i, i, i, "aa", "bb", "cc", "dd");
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_3_t::insert_row");
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_insert_txn_size)
{
    for (size_t txn_size : {1UL, 4UL, 8UL, 16UL, 256UL, 1024UL, 4096UL, 16384UL, c_max_insertion_single_txn})
    {
        const size_t num_records = (txn_size == 1) ? c_num_records / 10 : c_num_records;

        auto insert = [num_records, txn_size]() {
            bulk_insert(&simple_table_t::insert_row, num_records, txn_size);
        };

        bool clear_db_after_each_iteration = true;
        run_performance_test(
            insert,
            clear_database,
            gaia_fmt::format("simple_table_t::simple_table_insert_txn_size with txn of size {}", txn_size),
            clear_db_after_each_iteration,
            c_num_iterations, num_records);
    }
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_concurrent)
{
    for (size_t num_workers : {2, 4, 8})
    {
        auto insert = [num_workers]() {
            std::vector<std::thread> workers;

            for (size_t i = 0; i < num_workers; i++)
            {
                workers.emplace_back(insert_thread, (c_num_records / num_workers), c_max_insertion_single_txn);
            }

            for (auto& worker : workers)
            {
                worker.join();
            }
        };

        run_performance_test(
            insert,
            clear_database,
            gaia_fmt::format("simple_table_t::insert_row with {} threads", num_workers));
    }
}

TEST_F(direct_access__perf_insert_basic__test, simple_table_insert_txn_size_concurrent)
{
    for (size_t txn_size : {1, 4, 8, 128})
    {
        for (size_t num_workers : {2, 4, 8})
        {
            const size_t num_records = (txn_size == 1) ? c_num_records / 10 : c_num_records;

            auto insert = [num_workers, num_records, txn_size]() {
                std::vector<std::thread> workers;

                for (size_t i = 0; i < num_workers; i++)
                {
                    workers.emplace_back(insert_thread, (num_records / num_workers), txn_size);
                }

                for (auto& worker : workers)
                {
                    worker.join();
                }
            };

            bool clear_db_after_each_iteration = true;
            run_performance_test(
                insert,
                clear_database,
                gaia_fmt::format(
                    "simple_table_t::simple_table_insert_txn_size_concurrent threads:{} txn_size:{}",
                    num_workers,
                    txn_size),
                clear_db_after_each_iteration,
                c_num_iterations,
                num_records);
        }
    }
}
