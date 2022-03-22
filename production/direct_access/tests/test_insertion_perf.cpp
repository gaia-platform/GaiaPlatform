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

static const uint64_t c_num_insertion = 100000;

// This is a hard limit imposed by the db architecture.
static const uint64_t c_max_insertion_single_txn = (1 << 16) - 1;

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

/**
 * Consume an iterator.
 */
template <typename T_itr>
void consume_iter(T_itr& curr, T_itr end)
{
    while (curr != end)
    {
        curr++;
    }
}

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
            // TODO this increases the deletion time by quite a bit and with the current branch
            //  the 'Connection reset by peer' does not appear anymore.
            //  We may want to just delete this.
            // By consuming the iterator, before starting a new transaction, we avoid:
            // "Stream socket error: 'Connection reset by peer'."
            // consume_iter(obj_it, T_type::list().end());
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
    clear_table<simple_table_index_t>();
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
    uint64_t m_num_observations = 0;
};

double_t percentage_difference(int64_t expr, int64_t plain)
{
    return static_cast<double_t>(expr - plain) / plain * 100.0;
}

void log_performance_difference(accumulator_t<int64_t> expr_accumulator, std::string_view message, uint64_t num_insertions)
{
    double_t avg_expr = g_timer_t::ns_to_us(expr_accumulator.avg());
    double_t avg_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.avg());
    double_t min_expr = g_timer_t::ns_to_us(expr_accumulator.min());
    double_t max_expr = g_timer_t::ns_to_us(expr_accumulator.max());
    double_t single_record_insertion = avg_expr / num_insertions;

    cout << message << " performance:" << endl;
    printf(
        "  [expr]: avg:%0.2fus/%0.2fms min:%0.2fus max:%0.2fus single_insert:%0.2fus\n",
        avg_expr, avg_expr_ms, min_expr, max_expr, single_record_insertion);

    cout << endl;
}

void run_performance_test(
    std::function<void()> expr_fn,
    std::string_view message,
    size_t num_iterations = 5,
    uint64_t num_insertions = c_num_insertion)
{
    accumulator_t<int64_t> expr_accumulator;

    for (size_t iteration = 0; iteration < num_iterations; iteration++)
    {
        gaia_log::app().debug("[{}]: {} iteration staring, {} insertions", message, iteration, num_insertions);
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);

        double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
        gaia_log::app().debug("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

        gaia_log::app().debug("[{}]: {} iteration, clearing database", message, iteration);
        int64_t clear_database_duration = g_timer_t::get_function_duration(clear_database);
        double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
        gaia_log::app().debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
    }

    log_performance_difference(expr_accumulator, message, num_insertions);
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

TEST_F(test_insert_perf, DISABLED_simple_table_insert)
{
    auto insert1 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < c_num_insertion; i++)
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
    };

    run_performance_test(insert1, "simple_table_t::insert_row");
}

TEST_F(test_insert_perf, DISABLED_simple_table_writer)
{
    auto insert2 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < c_num_insertion; i++)
        {
            if (i > 0 && i % c_max_insertion_single_txn == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            simple_table_writer w;
            w.uint64_field = i;
            w.insert_row();
        }

        if (gaia::db::is_transaction_open())
        {
            gaia::db::commit_transaction();
        }
    };

    run_performance_test(insert2, "simple_table_writer");
}

TEST_F(test_insert_perf, DISABLED_simple_table_2)
{
    auto insert3 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < c_num_insertion; i++)
        {
            if (i > 0 && i % c_max_insertion_single_txn == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            simple_table_2_t::insert_row(i, "suppini", {1, 2, 3, 4, 5});
        }

        if (gaia::db::is_transaction_open())
        {
            gaia::db::commit_transaction();
        }
    };

    run_performance_test(insert3, "simple_table_2_t::insert_row");
}

TEST_F(test_insert_perf, DISABLED_simple_table_index)
{
    auto insert4 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < c_num_insertion; i++)
        {
            if (i > 0 && i % c_max_insertion_single_txn == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            simple_table_index_t::insert_row(i);
        }

        if (gaia::db::is_transaction_open())
        {
            gaia::db::commit_transaction();
        }
    };

    run_performance_test(insert4, "simple_table_index_t::insert_row");
}

TEST_F(test_insert_perf, DISABLED_simple_relationships)
{
    auto insert5 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < (c_num_insertion / 2); i++)
        {
            // Divides by 5 because the following operations causes 5 objects mutations.
            if (i > 0 && i % (c_max_insertion_single_txn / 5) == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            table_parent_t parent = table_parent_t::get(table_parent_t::insert_row());
            gaia_id_t child = table_child_t::insert_row();
            parent.children().insert(child);
        }

        if (gaia::db::is_transaction_open())
        {
            gaia::db::commit_transaction();
        }
    };

    run_performance_test(insert5, "simple_relationships");
}

TEST_F(test_insert_perf, DISABLED_value_linked_relationships)
{
    // VLR are so slow that we need to use a lower number of insertion to
    // finish in a reasonable amount of time.
    constexpr uint64_t c_vlr_insertions = c_num_insertion / 200;

    auto insert6 = []() {
        gaia::db::begin_transaction();

        for (size_t i = 0; i < c_vlr_insertions; i++)
        {
            if (i > 0 && i % (c_max_insertion_single_txn / 2) == 0)
            {
                gaia::db::commit_transaction();
                gaia::db::begin_transaction();
            }

            table_parent_vlr_t::insert_row(i);
            table_child_vlr_t::insert_row(i);
        }

        if (gaia::db::is_transaction_open())
        {
            gaia::db::commit_transaction();
        }
    };

    run_performance_test(insert6, "Value Linked Relationships", 5, c_vlr_insertions);
}

// To reproduce GAIAPLAT-1475 you need to:
// - c_num_insertion = 200000
// - Call run_performance_test with num_iterations=10
// Keeping it disabled for now because it takes too much time.
TEST_F(test_insert_perf, DISABLED_simple_table_concurrent)
{
    constexpr size_t c_num_workers = 5;

    auto insert = []() {
        std::vector<std::thread> workers;

        for (size_t i = 0; i < c_num_workers; i++)
        {
            workers.emplace_back(insert_thread, (c_num_insertion / c_num_workers));
        }

        for (auto& worker : workers)
        {
            worker.join();
        }
    };

    run_performance_test(insert, gaia_fmt::format("simple_table_t::insert_row with {} threads", c_num_workers));
}
