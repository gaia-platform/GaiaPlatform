/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_rel.h"
#include "test_perf.hpp"

using namespace gaia::perf_rel;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

class test_insert_perf_rel : public gaia::db::db_catalog_test_base_t
{
public:
    test_insert_perf_rel()
        : db_catalog_test_base_t("perf_rel.ddl"){};

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
    // Batch record deletions to avoid hitting transaction limits.
    constexpr size_t c_max_deletion_per_txn = 10000;
    clear_table<table_child_t>(c_max_deletion_per_txn);
    clear_table<table_parent_t>(c_max_deletion_per_txn);
    clear_table<table_child_vlr_t>(c_max_deletion_per_txn);
    clear_table<table_parent_vlr_t>(c_max_deletion_per_txn);
}

TEST_F(test_insert_perf_rel, simple_relationships)
{
    auto insert = []() {
        bulk_insert(
            [](size_t) {
                table_parent_t parent = table_parent_t::get(table_parent_t::insert_row());
                gaia_id_t child = table_child_t::insert_row();
                parent.children().insert(child);
            },
            c_num_records,
            c_max_insertion_single_txn / 5);
    };

    run_performance_test(
        insert, clear_database, "simple_relationships");
}

TEST_F(test_insert_perf_rel, value_linked_relationships_parent_only)
{
    // VLR are so slow that we need to use a lower number of insertions to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;
    constexpr size_t c_max_vlr_insertion_single_txn = 30000;

    auto insert = []() {
        bulk_insert(
            &table_parent_vlr_t::insert_row,
            c_vlr_insertions,
            c_max_vlr_insertion_single_txn);
    };

    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_parent_only",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions);
}

TEST_F(test_insert_perf_rel, value_linked_relationships_child_only)
{
    // VLR are so slow that we need to use a lower number of insertions to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;
    constexpr size_t c_max_vlr_insertion_single_txn = 30000;

    auto insert = []() {
        bulk_insert(
            &table_child_vlr_t::insert_row,
            c_vlr_insertions,
            c_max_vlr_insertion_single_txn);
    };

    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_child_only",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions);
}

TEST_F(test_insert_perf_rel, value_linked_relationships_autoconnect_to_same_parent_insert_parent_first)
{
    // VLR are so slow that we need to use a lower number of insertions to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 100;

    auto insert = []() {
        // Insert parent row.
        gaia::db::begin_transaction();
        table_parent_vlr_t::insert_row(0);
        gaia::db::commit_transaction();

        // Insert children rows.
        bulk_insert(
            [](size_t) { table_child_vlr_t::insert_row(0); },
            c_vlr_insertions);
    };

    // We perform c_vlr_insertions + 1 insertions.
    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_autoconnect_to_same_parent_insert_parent_first",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions + 1);
}

TEST_F(test_insert_perf_rel, value_linked_relationships_autoconnect_to_same_parent_insert_children_first)
{
    // A max limit of 20000 is imposed to avoid running into the per-txn object update limit.
    // The additional arithmetic makes the count scale with c_num_records, so that the test
    // takes a reasonable amount of time during regular runs.
    constexpr size_t c_vlr_insertions = std::min(20 * (c_num_records / 1000), 20000UL);

    auto insert = []() {
        // Insert children rows.
        bulk_insert(
            [](size_t) { table_child_vlr_t::insert_row(0); },
            c_vlr_insertions);

        // Insert parent row.
        gaia::db::begin_transaction();
        table_parent_vlr_t::insert_row(0);
        gaia::db::commit_transaction();
    };

    // We perform c_vlr_insertions + 1 insertions.
    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_autoconnect_to_same_parent_insert_children_first",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions + 1);
}

TEST_F(test_insert_perf_rel, value_linked_relationships_autoconnect_to_different_parent_insert_parent_first)
{
    // VLR are so slow that we need to use a lower number of insertions to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;
    constexpr size_t c_max_vlr_insertion_single_txn = 10000;

    auto insert = []() {
        bulk_insert(
            [](size_t iter) {
                table_parent_vlr_t::insert_row(iter);
                table_child_vlr_t::insert_row(iter);
            },
            c_vlr_insertions,
            c_max_vlr_insertion_single_txn);
    };

    // We perform c_vlr_insertions * 2 insertions.
    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_autoconnect_to_different_parent_insert_parent_first",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions * 2);
}

TEST_F(test_insert_perf_rel, value_linked_relationships_autoconnect_to_different_parent_insert_child_first)
{
    // VLR are so slow that we need to use a lower number of insertions to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;
    constexpr size_t c_max_vlr_insertion_single_txn = 10000;

    auto insert = []() {
        bulk_insert(
            [](size_t iter) {
                table_child_vlr_t::insert_row(iter);
                table_parent_vlr_t::insert_row(iter);
            },
            c_vlr_insertions,
            c_max_vlr_insertion_single_txn);
    };

    // We perform c_vlr_insertions * 2 insertions.
    bool clear_db_after_each_iteration = true;
    run_performance_test(
        insert,
        clear_database,
        "value_linked_relationships_autoconnect_to_different_parent_insert_child_first",
        clear_db_after_each_iteration,
        c_num_iterations,
        c_vlr_insertions * 2);
}
