/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cmath>
#include <cstdio>

#include <functional>
#include <iostream>
#include <limits>

#include "spdlog/spdlog.h"
#include <gtest/gtest.h>

#include "objectbox-model.h"
#include "objectbox.hpp"
#include "simple-table.obx.hpp"
#include "timer.hpp"

const constexpr uint32_t c_num_records = 10000000;
const constexpr uint32_t c_num_iterations = 3;
const constexpr uint32_t c_max_insertion_single_txn = c_num_records > 1000000 ? c_num_records / 10 : c_num_records;

using namespace std::chrono;
using namespace std;

using g_timer_t = gaia::common::timer_t;

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

class objectbox_bench : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        spdlog::set_level(spdlog::level::info);
    }

    static void TearDownTestCase()
    {
        spdlog::shutdown();
    }

    void SetUp() override
    {
        if (storage)
        {
            return;
        }

        storage = make_unique<obx::Store>(create_obx_model());
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
            "[%s] %lu rows, %zu iterations:\n"
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
        uint64_t num_insertions = c_num_records,
        bool read_benchmark = false)
    {
        accumulator_t<int64_t> expr_accumulator;

        for (size_t iteration = 0; iteration < num_iterations; iteration++)
        {
            spdlog::info("[{}]: {} iteration staring, {} insertions", message, iteration, num_insertions);
            int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
            expr_accumulator.add(expr_duration);

            double_t iteration_ms = g_timer_t::ns_to_ms(expr_duration);
            spdlog::info("[{}]: {} iteration, completed in {:.2f}ms", message, iteration, iteration_ms);

            // Writer benchmarks need to reset the data on every iteration.
            if (!read_benchmark)
            {
                spdlog::debug("[{}]: {} iteration, clearing database", message, iteration);
                int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(); });
                double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
                spdlog::debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
            }
        }

        // Reader benchmarks need to use the data on every iteration hence the deletion should happen only at the end.
        if (read_benchmark)
        {
            spdlog::debug("[{}]: clearing database", message);
            int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(); });
            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            spdlog::debug("[{}]: cleared in {:.2f}ms", message, clear_ms);
        }

        log_performance_difference(expr_accumulator, message, num_insertions, num_iterations);
    }

    void clear_database()
    {
        obx::Box<simple_table> box(*storage.get());
        box.removeAll();
    }

public:
    std::unique_ptr<obx::Store> storage;
};

TEST_F(objectbox_bench, simple_insert)
{
    auto simple_insert = [this]() {
        obx::Box<simple_table> box(*storage.get());
        std::unique_ptr<obx::Transaction> txn = make_unique<obx::Transaction>(storage->txWrite());

        for (int i = 0; i < c_num_records; i++)
        {
            if (i > 0 && i % c_max_insertion_single_txn == 0)
            {
                txn->success();
                txn = make_unique<obx::Transaction>(storage->txWrite());
            }

            box.put({.field = static_cast<uint64_t>(i)});
        }

        if (txn->isActive())
        {
            txn->success();
        }
    };

    run_performance_test(simple_insert, "objectbox::simple_bulk_insert");
}

TEST_F(objectbox_bench, simple_insert_txn_size)
{
    for (int txn_size : {1, 10, 100, 1000, 10000})
    {
        auto simple_insert = [this, txn_size]() {
            obx::Box<simple_table> box(*storage.get());
            std::unique_ptr<obx::Transaction> txn = make_unique<obx::Transaction>(storage->txWrite());

            for (int i = 0; i < 100000; i++)
            {
                if (i > 0 && i % txn_size == 0)
                {
                    txn->success();
                    txn = make_unique<obx::Transaction>(storage->txWrite());
                }

                box.put({.field = static_cast<uint64_t>(i)});
            }

            if (txn->isActive())
            {
                txn->success();
            }
        };

        run_performance_test(simple_insert, fmt::format("objectbox::simple_insert txn_size:{}", txn_size), 3, 100000);
    }
}
