/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <limits>
#include <string>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/logger.hpp"

#include "gaia_internal/common/timer.hpp"

using g_timer_t = gaia::common::timer_t;

static const uint64_t c_num_records = 100000;
static const uint64_t c_num_iterations = 1;

// This is a hard limit imposed by the db architecture.
static const uint64_t c_max_insertion_single_txn = (1 << 16) - 1;

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
    uint64_t m_num_observations = 0;
};

double_t percentage_difference(int64_t expr, int64_t plain)
{
    return static_cast<double_t>(expr - plain) / static_cast<double_t>(plain) * 100.0;
}

void log_performance_difference(accumulator_t<int64_t> expr_accumulator, std::string_view message, uint64_t num_insertions, size_t num_iterations)
{
    double_t avg_expr_us = g_timer_t::ns_to_us(static_cast<int64_t>(expr_accumulator.avg()));
    double_t min_expr_us = g_timer_t::ns_to_us(expr_accumulator.min());
    double_t max_expr_us = g_timer_t::ns_to_us(expr_accumulator.max());
    double_t avg_expr_ms = g_timer_t::ns_to_ms(static_cast<int64_t>(expr_accumulator.avg()));
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

    std::cout << std::endl;
}

void run_performance_test(
    std::function<void()> expr_fn,
    std::function<void()> clear_db_fn,
    std::string_view message,
    bool clean_db_after_each_iteration = true,
    size_t num_iterations = c_num_iterations,
    uint64_t num_records = c_num_records)
{
    accumulator_t<int64_t> expr_accumulator;

    for (size_t iteration = 0; iteration < num_iterations; iteration++)
    {
        gaia_log::app().info("[{}]: {} iteration starting, {} records", message, iteration, num_records);
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);

        double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
        gaia_log::app().info("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

        if (clean_db_after_each_iteration)
        {
            gaia_log::app().debug("[{}]: {} iteration, clearing database", message, iteration);
            int64_t clear_database_duration = g_timer_t::get_function_duration(clear_db_fn);
            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            gaia_log::app().debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
        }
    }

    if (!clean_db_after_each_iteration)
    {
        gaia_log::app().info("[{}]: clearing database", message);
        int64_t clear_database_duration = g_timer_t::get_function_duration(clear_db_fn);
        double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
        gaia_log::app().info("[{}]: cleared in {:.2f}ms", message, clear_ms);
    }

    log_performance_difference(expr_accumulator, message, num_records, num_iterations);
}

template <typename T_work>
void bulk_insert(T_work insert, int64_t num_records = c_num_records, int64_t max_insertion_single_txn = c_max_insertion_single_txn)
{
    gaia::db::begin_transaction();

    for (int64_t i = 0; i < num_records; i++)
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
void bulk_update(T_work update, int64_t max_insertion_single_txn = c_max_insertion_single_txn)
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
