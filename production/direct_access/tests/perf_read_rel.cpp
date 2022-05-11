/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_rel.h"
#include "test_perf.hpp"

using namespace gaia::benchmark;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::perf_rel;
using namespace std;

// Creating a relationship lead to 4/5 db operations, which reduces the number
// of records that can be inserted by a single transaction.
// This is why c_max_insertion_single_txn is divided by 5.
static const size_t c_max_insertion_single_txn_rel = c_max_insertion_single_txn / 5;

class direct_access__perf_read_rel : public gaia::db::db_catalog_test_base_t
{
public:
    direct_access__perf_read_rel()
        : db_catalog_test_base_t("perf_rel.ddl", true, true, true)
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
    constexpr size_t c_max_deletion_per_txn = c_max_insertion_single_txn_rel;
    clear_table<table_child_t>(c_max_deletion_per_txn);
    clear_table<table_parent_t>(c_max_deletion_per_txn);
    clear_table<table_child_vlr_t>(c_max_deletion_per_txn);
    clear_table<table_parent_vlr_t>(c_max_deletion_per_txn);

    clear_table<table_j3_t>(c_max_deletion_per_txn);
    clear_table<table_j2_t>(c_max_deletion_per_txn);
    clear_table<table_j1_t>(c_max_deletion_per_txn);
}

size_t insert_data(size_t num_children_per_parent = c_num_records - 1)
{
    gaia::db::begin_transaction();
    size_t total_records_inserted = 0;

    for (size_t i = 0; i < c_num_records; i++)
    {

        if (total_records_inserted > 0 && total_records_inserted % c_max_insertion_single_txn_rel == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
        }

        if (i + 1 + (num_children_per_parent) > c_num_records)
        {
            break;
        }

        table_parent_t parent = table_parent_t::get(table_parent_t::insert_row());
        total_records_inserted++;

        for (size_t j = 0; j < num_children_per_parent; j++)
        {
            if ((total_records_inserted % c_max_insertion_single_txn_rel) == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            gaia_id_t child = table_child_t::insert_row();
            parent.children().insert(child);
            total_records_inserted++;
        }
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }

    return total_records_inserted;
}

TEST_F(direct_access__perf_read_rel, single_join)
{
    const size_t num_records_inserted = insert_data();

    benchmark_fn_t work_fn = [num_records_inserted]() {
        gaia::db::begin_transaction();

        size_t i = 0;
        for (const auto& parent : table_parent_t::list())
        {
            i++;
            for ([[maybe_unused]] auto& child : parent.children())
            {
                i++;
            }
        }

        gaia::db::commit_transaction();

        ASSERT_EQ(num_records_inserted, i);
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        work_fn, clear_database, "simple_table_t::table_scan", clear_db_after_each_iteration, c_num_iterations, num_records_inserted);
}

size_t insert_nested_join_data(size_t num_records = c_num_records)
{
    // This benchmark created 3 tables, connected by two relationships:
    // table_j1 -> table_j2 -> table_j3.
    // The cubic root of num_records to create to calculate how many records to create
    // for each join staying within the num_records boundary:
    //  c_num_records_per_join^3 ~ num_records
    //
    // We create c_num_records_per_join table_j1 records, for each table_j1 record
    // we create c_num_records_per_join table_j2 records, same for table_j3.
    const auto c_num_records_per_join = static_cast<size_t>(std::cbrt(num_records));
    size_t num_j3_records = 0;

    gaia::db::begin_transaction();

    for (size_t i = 1; i <= c_num_records_per_join; i++)
    {

        table_j1_t j1 = table_j1_t::get(table_j1_t::insert_row());

        for (size_t j = 1; j <= c_num_records_per_join; j++)
        {
            table_j2_t j2 = table_j2_t::get(table_j2_t::insert_row());
            j1.j2().insert(j2);

            for (size_t z = 1; z <= c_num_records_per_join; z++)
            {
                if (i * j * z >= c_max_insertion_single_txn_rel)
                {
                    gaia::db::commit_transaction();
                    gaia::db::begin_transaction();
                }

                table_j3_t j3 = table_j3_t::get(table_j3_t::insert_row());
                j2.j3().insert(j3);
                num_j3_records++;
            }
        }
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }

    return num_j3_records;
}

TEST_F(direct_access__perf_read_rel, nested_joins)
{
    for (size_t num_records : {10UL, 100UL, 1000UL, 10000UL, 100000UL})
    {
        clear_database();
        const size_t num_j3_records = insert_nested_join_data(num_records);

        benchmark_fn_t work_fn = [num_j3_records]() {
            gaia::db::begin_transaction();

            size_t i = 0;
            for (const auto& j1 : table_j1_t::list())
            {
                for (const auto& j2 : j1.j2())
                {
                    for ([[maybe_unused]] const auto& j3 : j2.j3())
                    {
                        i++;
                    }
                }
            }

            gaia::db::commit_transaction();

            ASSERT_EQ(num_j3_records, i);
        };

        bool clear_db_after_each_iteration = false;
        run_performance_test(
            work_fn,
            clear_database,
            gaia_fmt::format("simple_table_t::table_scan num_records:{}", num_records),
            clear_db_after_each_iteration,
            c_num_iterations,
            num_j3_records);
    }
}
