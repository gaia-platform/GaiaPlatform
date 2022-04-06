/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_insert_sandbox.h"

using namespace gaia::insert_sandbox;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace std;

using g_timer_t = gaia::common::timer_t;

static const size_t c_num_records = 100000;
static const size_t c_num_iterations = 1;

// This is a hard limit imposed by the db architecture.
static const size_t c_max_insertion_single_txn = (1 << 16) - 1;

class test_insert_perf : public gaia::db::db_catalog_test_base_t
{
public:
    test_insert_perf()
        : db_catalog_test_base_t("insert.ddl"){};

    void TearDown() override
    {
        if (gaia::db::is_transaction_open())
        {
            gaia::db::rollback_transaction();
        }

        db_catalog_test_base_t::TearDown();
    }
};

template <typename T_type>
void clear_table(size_t max_deletion_per_txn = c_max_insertion_single_txn)
{
    size_t counter = 1;
    gaia::db::begin_transaction();

    for (auto obj_it = T_type::list().begin();
         obj_it != T_type::list().end();)
    {
        auto next_obj_it = obj_it++;
        next_obj_it->delete_row();

        if (counter % max_deletion_per_txn == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
            // Avoid "Cursor was not called from the scope of its own transaction!" thrown in the test body."
            obj_it = T_type::list().begin();
        }
        counter++;
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }
}

void clear_database()
{
    clear_table<simple_table_t>();
    clear_table<simple_table_2_t>();
    clear_table<simple_table_3_t>();
    clear_table<unique_index_table_t>();
    clear_table<hash_index_table_t>();
    clear_table<range_index_table_t>();
    // When deleting a connected entity there are 4 objects mutations happening.
    clear_table<table_child_t>(c_max_insertion_single_txn / 4);
    clear_table<table_parent_t>();
    clear_table<table_child_vlr_t>();
    clear_table<table_parent_vlr_t>();
}

template <typename T_num>
class accumulator_t
{
public:
    void add(T_num value)
    {
        if (value < m_min)
        {
            m_min = value;
        }

        if (value > m_max)
        {
            m_max = value;
        }

        m_sum += value;
        m_num_observations++;
    }

    T_num min()
    {
        return m_min;
    }

    T_num max()
    {
        return m_max;
    }

    double_t avg()
    {
        return m_sum / (m_num_observations + 0.0);
    }

private:
    T_num m_min = std::numeric_limits<T_num>::max();
    T_num m_max = std::numeric_limits<T_num>::min();
    T_num m_sum = 0;
    size_t m_num_observations = 0;
};

double_t percentage_difference(size_t expr, size_t plain)
{
    return static_cast<double_t>(expr - plain) / static_cast<double_t>(plain) * 100.0;
}

void log_performance_difference(accumulator_t<size_t> expr_accumulator, std::string_view message, size_t num_insertions, size_t num_iterations)
{
    double_t avg_expr_us = g_timer_t::ns_to_us(static_cast<size_t>(expr_accumulator.avg()));
    double_t min_expr_us = g_timer_t::ns_to_us(expr_accumulator.min());
    double_t max_expr_us = g_timer_t::ns_to_us(expr_accumulator.max());
    double_t avg_expr_ms = g_timer_t::ns_to_ms(static_cast<size_t>(expr_accumulator.avg()));
    double_t min_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.min());
    double_t max_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.max());

    double_t single_record_avg_ns = avg_expr_us / static_cast<double_t>(num_insertions);
    double_t single_record_min_ns = min_expr_us / static_cast<double_t>(num_insertions);
    double_t single_record_max_ns = max_expr_us / static_cast<double_t>(num_insertions);

    printf(
        "[%s] %lu records, %zu iterations:\n"
        "   [total]: avg:%0.2fms min:%0.2fms max:%0.2fms\n"
        "  [single]: avg:%0.2fus min:%0.2fus max:%0.2fus",
        message.data(), num_insertions, num_iterations,
        avg_expr_ms, min_expr_ms, max_expr_ms,
        single_record_avg_ns, single_record_min_ns, single_record_max_ns);

    cout << endl;
}

void run_performance_test(
    std::function<void()> expr_fn,
    std::string_view message,
    size_t num_iterations = c_num_iterations,
    size_t num_insertions = c_num_records)
{
    accumulator_t<size_t> expr_accumulator;

    for (size_t iteration = 0; iteration < num_iterations; iteration++)
    {
        gaia_log::app().info("[{}]: {} iteration starting, {} records", message, iteration, num_insertions);
        size_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);

        double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
        gaia_log::app().info("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

        gaia_log::app().debug("[{}]: {} iteration, clearing database", message, iteration);
        size_t clear_database_duration = g_timer_t::get_function_duration(clear_database);
        double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
        gaia_log::app().debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
    }

    log_performance_difference(expr_accumulator, message, num_insertions, num_iterations);
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

template <typename T_work>
void bulk_insert(T_work insert, size_t num_records = c_num_records, size_t max_insertion_single_txn = c_max_insertion_single_txn)
{
    gaia::db::begin_transaction();

    for (size_t i = 0; i < num_records; i++)
    {
        if (i > 0 && i % max_insertion_single_txn == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
        }

        insert(i);
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }
}

TEST_F(test_insert_perf, simple_table_insert)
{
    auto insert = []() {
        bulk_insert(&simple_table_t::insert_row);
    };

    run_performance_test(insert, "simple_table_t::insert_row");
}

TEST_F(test_insert_perf, simple_table_writer)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_writer w;
            w.uint64_field = i;
            w.insert_row();
        });
    };

    run_performance_test(insert, "simple_table_writer");
}

TEST_F(test_insert_perf, simple_table_2)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_2_t::insert_row(i, "suppini", {1, 2, 3, 4, 5});
        });
    };

    run_performance_test(insert, "simple_table_2_t::insert_row");
}

TEST_F(test_insert_perf, simple_table_3)
{
    auto insert = []() {
        bulk_insert([](size_t i) {
            simple_table_3_t::insert_row(
                i, i, i, i, "aa", "bb", "cc", "dd");
        });
    };

    run_performance_test(insert, "simple_table_3_t::insert_row");
}

TEST_F(test_insert_perf, simple_table_insert_txn_size)
{
    for (size_t txn_size : {1, 4, 8, 16, 256, 1024, 4096, 16384, static_cast<int>(c_max_insertion_single_txn)})
    {
        const size_t num_records = txn_size == 1 ? c_num_records / 10 : c_num_records;

        auto insert = [txn_size, num_records]() {
            // Insertion with txn_size == 1 is really slow, hence reducing the number records.
            bulk_insert(&simple_table_t::insert_row, num_records, txn_size);
        };

        run_performance_test(
            insert,
            gaia_fmt::format("simple_table_t::simple_table_insert_txn_size with txn of size {}", txn_size),
            c_num_iterations, num_records);
    }
}

TEST_F(test_insert_perf, unique_index_table)
{
    auto insert = []() {
        bulk_insert(&unique_index_table_t::insert_row);
    };

    run_performance_test(insert, "unique_index_table_t::insert_row");
}

TEST_F(test_insert_perf, hash_index_table)
{
    auto insert = []() {
        bulk_insert(&hash_index_table_t::insert_row);
    };

    run_performance_test(insert, "hash_index_table::insert_row");
}

TEST_F(test_insert_perf, range_index_table)
{
    auto insert = []() {
        bulk_insert(&range_index_table_t::insert_row);
    };

    run_performance_test(insert, "range_index_table::insert_row");
}

TEST_F(test_insert_perf, simple_relationships)
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

    run_performance_test(insert, "simple_relationships");
}

TEST_F(test_insert_perf, value_linked_relationships_parent_only)
{
    // VLR are so slow that we need to use a lower number of insertion to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;

    auto insert = []() {
        bulk_insert(
            &table_parent_vlr_t::insert_row,
            c_vlr_insertions);
    };

    run_performance_test(insert, "value_linked_relationships_parent_only", c_num_iterations, c_vlr_insertions);
}

TEST_F(test_insert_perf, value_linked_relationships_child_only)
{
    // VLR are so slow that we need to use a lower number of insertion to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;

    auto insert = []() {
        bulk_insert(
            &table_child_vlr_t::insert_row,
            c_vlr_insertions);
    };

    run_performance_test(insert, "value_linked_relationships_child_only", c_num_iterations, c_vlr_insertions);
}

TEST_F(test_insert_perf, value_linked_relationships_autoconnect_to_same_parent)
{
    // VLR are so slow that we need to use a lower number of insertion to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 50;

    auto insert = []() {
        gaia::db::begin_transaction();
        table_parent_vlr_t::insert_row(0);
        gaia::db::commit_transaction();
        bulk_insert(
            [](size_t) { table_child_vlr_t::insert_row(0); },
            c_vlr_insertions,
            c_max_insertion_single_txn / 2);
    };

    run_performance_test(insert, "value_linked_relationships_autoconnect_to_same_parent", c_num_iterations, c_vlr_insertions + 1);
}

TEST_F(test_insert_perf, value_linked_relationships_autoconnect_to_different_parent)
{
    // VLR are so slow that we need to use a lower number of insertion to
    // finish in a reasonable amount of time.
    constexpr size_t c_vlr_insertions = c_num_records / 10;

    auto insert = []() {
        bulk_insert(
            [](size_t i) {
                table_parent_vlr_t::insert_row(i);
                table_child_vlr_t::insert_row(i);
            },
            c_vlr_insertions,
            c_max_insertion_single_txn / 5);
    };

    run_performance_test(insert, "value_linked_relationships_autoconnect_to_different_parent", c_num_iterations, c_vlr_insertions * 2);
}

TEST_F(test_insert_perf, simple_table_concurrent)
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

        run_performance_test(insert, gaia_fmt::format("simple_table_t::insert_row with {} threads", num_workers));
    }
}

TEST_F(test_insert_perf, simple_table_insert_txn_size_concurrent)
{
    for (size_t txn_size : {1, 4, 8, 128})
    {
        for (size_t num_workers : {2, 4, 8})
        {
            const size_t num_records = txn_size == 1 ? c_num_records / 10 : c_num_records;

            auto insert = [num_workers, txn_size, num_records]() {
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

            run_performance_test(
                insert,
                gaia_fmt::format("simple_table_t::simple_table_insert_txn_size_concurrent threads:{} txn_size:{}", num_workers, txn_size),
                c_num_iterations, num_records);
        }
    }
}
