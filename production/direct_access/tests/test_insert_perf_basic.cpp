/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_basic.h"
#include "test_perf.hpp"

using namespace gaia::perf_basic;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

class test_insert_perf_basic : public gaia::db::db_catalog_test_base_t
{
public:
    test_insert_perf_basic()
        : db_catalog_test_base_t("perf_basic.ddl"){};

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

void insert_thread(size_t num_records)
{
    gaia::db::begin_session();
    gaia::db::begin_transaction();

    for (size_t i = 0; i < num_records; i++)
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

    gaia::db::end_session();
}

TEST_F(test_insert_perf_basic, simple_table_insert)
{
    auto insert = []() {
        bulk_insert(&simple_table_t::insert_row);
    };

    run_performance_test(
        insert, clear_database, "simple_table_t::insert_row");
}

TEST_F(test_insert_perf_basic, simple_table_writer)
{
    auto insert = []() {
        bulk_insert([](size_t iter) {
            simple_table_writer w;
            w.uint64_field = iter;
            w.insert_row();
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_writer");
}

TEST_F(test_insert_perf_basic, simple_table_2)
{
    auto insert = []() {
        bulk_insert([](size_t iter) {
            simple_table_2_t::insert_row(iter, "suppini", {1, 2, 3, 4, 5});
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_2_t::insert_row");
}

TEST_F(test_insert_perf_basic, simple_table_3)
{
    auto insert = []() {
        bulk_insert([](size_t iter) {
            simple_table_3_t::insert_row(
                iter, iter, iter, iter, "aa", "bb", "cc", "dd");
        });
    };

    run_performance_test(
        insert, clear_database, "simple_table_3_t::insert_row");
}

TEST_F(test_insert_perf_basic, simple_table_concurrent)
{
    for (size_t num_workers : {2, 4, 8})
    {
        auto insert = [num_workers]() {
            std::vector<std::thread> workers;

            for (size_t i = 0; i < num_workers; i++)
            {
                workers.emplace_back(insert_thread, (c_num_records / num_workers));
            }

            for (auto& worker : workers)
            {
                worker.join();
            }
        };

        run_performance_test(
            insert, clear_database, gaia_fmt::format("simple_table_t::insert_row with {} threads", num_workers));
    }
}
