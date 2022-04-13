/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cmath>
#include <cstdio>

#include <chrono>
#include <functional>
#include <iostream>
#include <limits>

#include "spdlog/spdlog.h"
#include <gtest/gtest.h>
#include <sqlite_orm/sqlite_orm.h>

#include "timer.hpp"

const constexpr size_t c_num_records = 100000;
const constexpr size_t c_num_iterations = 5;
const constexpr size_t c_max_insertion_single_txn = c_num_records > 1000000 ? c_num_records / 10 : c_num_records;

using namespace std::chrono;
using namespace std;
using namespace sqlite_orm;

using g_timer_t = gaia::common::timer_t;

struct simple_table_t
{
    uint64_t uint64_field;
};

// Normal JOINS
struct table_parent_t
{
    uint64_t id;
};

struct table_child_t
{
    uint64_t parent_id;
};

// Nested joins
struct table_j1_t
{
    uint64_t id;
};

struct table_j2_t
{
    uint64_t id;
    uint64_t j1_id;
};

struct table_j3_t
{
    uint64_t id;
    uint64_t j2_id;
};

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

static inline auto init_storage()
{
    return make_storage(
        "file:cachedb?mode=memory&cache=shared",
        make_index("idx_j1_id", &table_j2_t::j1_id),
        make_index("idx_j2_id", &table_j3_t::j2_id),
        make_table(
            "simple_table",
            make_column("uint64_field", &simple_table_t::uint64_field)),
        make_table(
            "table_parent",
            make_column("id", &table_parent_t::id)),
        make_table(
            "table_child",
            make_column("parent_id", &table_child_t::parent_id)),
        make_table(
            "table_j1",
            make_column("id", &table_j1_t::id, primary_key())),
        make_table(
            "table_j2",
            make_column("id", &table_j2_t::id, primary_key()),
            make_column("j1_id", &table_j2_t::j1_id),
            foreign_key(&table_j2_t::j1_id).references(&table_j1_t::id)),
        make_table(
            "table_j3",
            make_column("id", &table_j3_t::id, primary_key()),
            make_column("j2_id", &table_j3_t::j2_id),
            foreign_key(&table_j3_t::j2_id).references(&table_j2_t::id)));
}

class sqlite_orm_bench : public ::testing::Test
{
public:
    using storage_t = decltype(init_storage());

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

        storage = make_unique<storage_t>(init_storage());
        storage->open_forever();
        storage->sync_schema();
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
            //            if (!read_benchmark)
            //            {
            //                spdlog::debug("[{}]: {} iteration, clearing database", message, iteration);
            //                int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(); });
            //                double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
            //                spdlog::debug("[{}]: {} iteration, cleared in {:.2f}ms", message, iteration, clear_ms);
            //            }
        }

        // Reader benchmarks need to use the data on every iteration hence the deletion should happen only at the end.
        //        if (read_benchmark)
        //        {
        //            spdlog::debug("[{}]: clearing database", message);
        //            int64_t clear_database_duration = g_timer_t::get_function_duration([this]() { clear_database(); });
        //            double_t clear_ms = g_timer_t::ns_to_ms(clear_database_duration);
        //            spdlog::debug("[{}]: cleared in {:.2f}ms", message, clear_ms);
        //        }

        log_performance_difference(expr_accumulator, message, num_insertions, num_iterations);
    }

    void clear_database()
    {
        storage->remove_all<simple_table_t>();
        storage->remove_all<table_parent_t>();
        storage->remove_all<table_child_t>();
        storage->remove_all<table_j3_t>();
        storage->remove_all<table_j2_t>();
        storage->remove_all<table_j1_t>();
    }

public:
    std::unique_ptr<storage_t> storage;
};

TEST_F(sqlite_orm_bench, simple_bulk_insert)
{
    auto simple_insert = [this]() {
        storage->begin_transaction();

        for (size_t i = 0; i < c_num_records; i++)
        {
            if (i > 0 && i % c_max_insertion_single_txn == 0)
            {
                storage->commit();
                storage->begin_transaction();
            }

            storage->insert(
                simple_table_t{
                    .uint64_field = static_cast<uint64_t>(i)});
        }

        if (storage->is_opened())
        {
            storage->commit();
        }
    };

    run_performance_test(simple_insert, "sqlite_orm::simple_insert");
}

TEST_F(sqlite_orm_bench, simple_bulk_insert_txn_size)
{
    for (size_t txn_size : {1, 10, 100, 1000, 10000})
    {
        size_t num_insertions = c_num_records;

        if (txn_size == 1)
        {
            num_insertions = num_insertions / 10;
        }

        auto simple_insert = [this, txn_size, num_insertions]() {
            storage->begin_transaction();

            for (size_t i = 0; i < num_insertions; i++)
            {
                if (i > 0 && i % txn_size == 0)
                {
                    storage->commit();
                    storage->begin_transaction();
                }

                storage->insert(
                    simple_table_t{
                        .uint64_field = static_cast<uint64_t>(i)});
            }

            if (storage->is_opened())
            {
                storage->commit();
            }
        };
        run_performance_test(simple_insert, fmt::format("sqlite_orm::simple_insert txn_size:{}", txn_size), c_num_iterations, num_insertions);
    }
}

TEST_F(sqlite_orm_bench, simple_bulk_insert_single_txn)
{
    auto simple_insert = [this]() {
        storage->transaction([&] {
            for (size_t i = 0; i < c_num_records; i++)
            {
                storage->insert(
                    simple_table_t{
                        .uint64_field = static_cast<uint64_t>(i)});
            }
            return true;
        });
    };

    run_performance_test(simple_insert, "sqlite_orm::simple_insert");
}

TEST_F(sqlite_orm_bench, table_scan)
{
    storage->transaction([&] {
        for (size_t i = 0; i < c_num_records; i++)
        {
            storage->insert(
                simple_table_t{
                    .uint64_field = static_cast<uint64_t>(i)});
        }
        return true;
    });

    for (size_t num_reads : {10UL, 100UL, 1000UL, 100000UL, 1000000UL, c_num_records})
    {
        auto table_scan = [this, num_reads]() {
            int i = 0;
            storage->transaction([&] {
                for (auto& item : storage->iterate<simple_table_t>())
                {
                    i++;
                    (void)item.uint64_field;
                    if (i == num_reads)
                    {
                        break;
                    }
                }
                return true;
            });

            ASSERT_EQ(i, num_reads);
        };

        run_performance_test(table_scan, fmt::format("sqlite_orm::table_scan num_reads={}", num_reads), num_reads < 1000 ? c_num_iterations * 10 : c_num_iterations, num_reads);
    }
}

TEST_F(sqlite_orm_bench, join)
{
    storage->transaction([&] {
        storage->insert(
            table_parent_t{
                .id = 0});

        for (size_t i = 0; i < c_num_records - 1; i++)
        {
            storage->insert(
                table_child_t{
                    .parent_id = 0});
        }
        return true;
    });

    auto table_scan = [this]() {
        int i = 0;

        storage->transaction([&] {
            auto result = storage->get_all<table_child_t>(join<table_parent_t>(on(c(&table_parent_t::id) == 0)));
            for (auto& item : result)
            {
                i++;
            }
            return true;
        });
        ASSERT_EQ(i, c_num_records - 1);
    };

    run_performance_test(table_scan, "sqlite_orm::join");
}

template <typename T_storage>
size_t insert_nested_join_data(T_storage& storage, size_t num_records_per_join)
{
    size_t num_j3_records = 0;

    storage->transaction([&] {
        for (size_t i = 0; i < num_records_per_join; i++)
        {
            uint64_t j1_id = storage->insert(
                table_j1_t{});

            for (size_t j = 0; j < num_records_per_join; j++)
            {
                uint64_t j2_id = storage->insert(
                    table_j2_t{
                        .j1_id = j1_id});

                for (size_t z = 0; z < num_records_per_join; z++)
                {
                    storage->insert(
                        table_j3_t{
                            .j2_id = j2_id});
                    num_j3_records++;
                }
            }
        }

        return true;
    });

    return num_j3_records;
}

TEST_F(sqlite_orm_bench, nested_join_query)
{
    for (size_t num_records : {10UL, 100UL, 1000UL, 10000UL, 100000UL})
    {
        clear_database();
        const auto c_num_records_per_join = static_cast<size_t>(std::cbrt(num_records));
        const auto c_tot_records = static_cast<size_t>(std::pow(c_num_records_per_join, 3));
        size_t num_j3_records = insert_nested_join_data(storage, c_num_records_per_join);

        auto j1_ids = storage->select(columns(&table_j1_t::id));

        auto table_scan = [this, num_j3_records, &j1_ids]() {
            size_t count = 0;

            storage->transaction([&] {
                for (auto& tpl : j1_ids)
                {
                    auto result = storage->select(
                        distinct(columns(&table_j3_t::id, &table_j3_t::j2_id)),
                        join<table_j2_t>(on(c(&table_j2_t::id) == &table_j3_t::j2_id)),
                        join<table_j1_t>(on(c(&table_j2_t::j1_id) == std::get<0>(tpl))));

                    for (auto& item : result)
                    {
                        count++;
                    }
                }

                return true;
            });

            ASSERT_EQ(count, num_j3_records);
        };

        run_performance_test(table_scan, fmt::format("sqlite_orm::nested_join_query num_records:{}", c_tot_records), c_num_iterations, num_j3_records);
    }
}

TEST_F(sqlite_orm_bench, nested_join_for_loops)
{
    for (size_t num_records : {10UL, 100UL, 1000UL, 10000UL, 100000UL})
    {
        clear_database();
        const auto c_num_records_per_join = static_cast<size_t>(std::cbrt(num_records));
        const auto c_tot_records = static_cast<size_t>(std::pow(c_num_records_per_join, 3));
        size_t num_j3_records = insert_nested_join_data(storage, c_num_records_per_join);

        auto table_scan = [this, num_j3_records]() {
            size_t count = 0;

            storage->transaction([&] {
                for (auto& j1 : storage->get_all<table_j1_t>())
                {
                    for (auto& j2 : storage->get_all<table_j2_t>(where(c(&table_j2_t::j1_id) == j1.id)))
                    {
                        for (auto& j3 : storage->get_all<table_j3_t>(where(c(&table_j3_t::j2_id) == j2.id)))
                        {
                            count++;
                        }
                    }
                }

                return true;
            });

            ASSERT_EQ(count, num_j3_records);
        };

        run_performance_test(table_scan, fmt::format("sqlite_orm::nested_join_for_loops num_records:{}", c_tot_records), c_num_iterations, num_j3_records);
    }
}
