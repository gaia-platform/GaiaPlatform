////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <limits>
#include <string>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/common/assert.hpp"
#include "gaia_internal/common/timer.hpp"

namespace gaia
{
namespace benchmark
{

using g_timer_t = gaia::common::timer_t;

static const size_t c_num_records = 1000;
static const size_t c_num_iterations = 1;

// This is a hard limit imposed by the db architecture.
static const size_t c_max_insertion_single_txn = (1UL << 16) - 1;

// Type declaration for the function that contains the benchmarking logic.
using benchmark_fn_t = std::function<void()>;

size_t get_duration(benchmark_fn_t fn)
{
    int64_t duration = g_timer_t::get_function_duration(fn);
    ASSERT_INVARIANT(duration >= 0, "g_timer_t::get_function_duration() has returned a negative value!");

    uint64_t result = duration;
    return result;
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
    return ((static_cast<double_t>(expr) - static_cast<double_t>(plain)) / static_cast<double_t>(plain)) * 100.0;
}

void log_performance_difference(
    accumulator_t<size_t> expr_accumulator,
    std::string_view message,
    size_t num_insertions,
    size_t num_iterations)
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
        "  [single]: avg:%0.2fus min:%0.2fus max:%0.2fus\n",
        message.data(), num_insertions, num_iterations,
        avg_expr_ms, min_expr_ms, max_expr_ms,
        single_record_avg_ns, single_record_min_ns, single_record_max_ns);
}

void run_performance_test(
    benchmark_fn_t expr_fn,
    benchmark_fn_t clear_db_fn,
    std::string_view message,
    bool clear_db_after_each_iteration = true,
    size_t num_iterations = c_num_iterations,
    size_t num_records = c_num_records)
{
    accumulator_t<size_t> expr_accumulator;

    for (size_t iteration = 1; iteration <= num_iterations; ++iteration)
    {
        gaia_log::app().info("[{}]: iteration #{} starting, {} records", message, iteration, num_records);
        size_t expr_duration = get_duration(expr_fn);
        expr_accumulator.add(expr_duration);

        double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
        gaia_log::app().info("[{}]: iteration #{}, completed in {:.2f}ms", message, iteration, iteration_ms);

        if (clear_db_after_each_iteration)
        {
            gaia_log::app().info("[{}]: iteration #{}, clearing database", message, iteration);
            size_t clear_database_duration = get_duration(clear_db_fn);
            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            gaia_log::app().info("[{}]: iteration #{}, cleared in {:.2f}ms", message, iteration, clear_ms);
        }
    }

    if (!clear_db_after_each_iteration)
    {
        gaia_log::app().info("[{}]: clearing database", message);
        size_t clear_database_duration = get_duration(clear_db_fn);
        double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
        gaia_log::app().info("[{}]: cleared in {:.2f}ms", message, clear_ms);
    }

    log_performance_difference(expr_accumulator, message, num_records, num_iterations);
}

template <typename T_work>
void bulk_insert(
    T_work insert,
    size_t num_records = c_num_records,
    size_t max_insertion_single_txn = c_max_insertion_single_txn)
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

template <typename T_type, typename T_work>
void bulk_update(
    T_work update,
    size_t max_insertion_single_txn = c_max_insertion_single_txn)
{
    gaia::db::begin_transaction();

    size_t i = 0;
    for (T_type& obj : T_type::list())
    {
        if (i > 0 && i % max_insertion_single_txn == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
        }

        update(obj);

        i++;
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }
}

} // namespace benchmark
} // namespace gaia
