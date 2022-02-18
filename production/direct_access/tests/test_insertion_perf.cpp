/////////////////////////////////////////////
/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.

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

static const int64_t c_num_insertion = 100000;
static const int64_t c_max_insertion_single_txn = 50000;

class test_insert_perf : public gaia::db::db_catalog_test_base_t
{
public:
    test_insert_perf()
        : db_catalog_test_base_t("insert.ddl"){};
};

template <typename T_itr>
void consume_iter(T_itr& curr, T_itr end)
{
    while (curr != end)
    {
        curr++;
    }
}

template <typename T_type>
void clear_table()
{
    size_t counter = 1;
    gaia::db::begin_transaction();

    for (auto obj_it = T_type::list().begin();
         obj_it != T_type::list().end();)
    {
        auto next_obj_it = obj_it++;
        next_obj_it->delete_row();

        if (counter % (c_max_insertion_single_txn / 2) == 0)
        {
            // By consuming the iterator, before starting a new transaction, we avoid:
            // "Stream socket error: 'Connection reset by peer'."
            consume_iter(obj_it, T_type::list().end());
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
    clear_table<table_child_t>();
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

void log_performance_difference(accumulator_t<int64_t> expr_accumulator, std::string_view message)
{
    double_t avg_expr = g_timer_t::ns_to_us(expr_accumulator.avg());
    double_t avg_expr_ms = g_timer_t::ns_to_ms(expr_accumulator.avg());
    double_t min_expr = g_timer_t::ns_to_us(expr_accumulator.min());
    double_t max_expr = g_timer_t::ns_to_us(expr_accumulator.max());
    double_t single_record_insertion = -1;

    single_record_insertion = avg_expr / c_num_insertion;

    cout << message << " performance:" << endl;
    printf(
        "  [expr]: avg:%0.2fus/%0.2fms min:%0.2fus max:%0.2fus single_insert:%0.2fus\n",
        avg_expr, avg_expr_ms, min_expr, max_expr, single_record_insertion);

    cout << endl;
}

void run_performance_test(
    std::function<void()> expr_fn,
    std::string_view message,
    uint32_t num_iterations = 5)
{
    accumulator_t<int64_t> expr_accumulator;

    for (uint32_t iteration = 0; iteration < num_iterations; iteration++)
    {
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);
        clear_database();
    }

    log_performance_difference(expr_accumulator, message);
}

void insert_thread(int num_records)
{
    gaia::db::begin_session();
    gaia::db::begin_transaction();

    for (int i = 0; i < num_records; i++)
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

// TODO re-enable tests. For now we just need to ensure that "Cannot find an object with ID '1196671'."
//  no longer happens.
TEST_F(test_insert_perf, insert)
{
    //    cout << "Inserting " << c_num_insertion << " objects in the database for each test" << endl;

    //    auto insert1 = []() {
    //        gaia::db::begin_transaction();
    //
    //        for (int i = 0; i < c_num_insertion; i++)
    //        {
    //            if (i > 0 && i % c_max_insertion_single_txn == 0)
    //            {
    //                gaia::db::commit_transaction();
    //                gaia::db::begin_transaction();
    //            }
    //
    //            simple_table_t::insert_row(i);
    //            cnt++;
    //        }
    //
    //        if (gaia::db::is_transaction_open())
    //        {
    //            gaia::db::commit_transaction();
    //        }
    //    };
    //
    //    run_performance_test(insert1, "simple_table_t::insert_row", 50);
    //
    //    auto insert2 = []() {
    //        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    //
    //        for (int i = 0; i < c_num_insertion; i++)
    //        {
    //            simple_table_writer w;
    //            w.uint64_field = i;
    //            w.insert_row();
    //        }
    //
    //        txn.commit();
    //    };
    //
    //    run_performance_test(insert2, "simple_table_writer");
    //
    //    auto insert3 = []() {
    //        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    //
    //        for (int i = 0; i < c_num_insertion; i++)
    //        {
    //            simple_table_2_t::insert_row(i, "suppini", {1, 2, 3, 4, 5});
    //        }
    //
    //        txn.commit();
    //    };
    //
    //    run_performance_test(insert3, "simple_table_2_t::insert_row");
    //
    //    auto insert4 = []() {
    //        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    //        for (int i = 0; i < c_num_insertion; i++)
    //        {
    //            simple_table_index_t::insert_row(i);
    //        }
    //        txn.commit();
    //    };
    //
    //    run_performance_test(insert4, "simple_table_index_t::insert_row");
    //
    //    auto insert5 = []() {
    //        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    //        for (int i = 0; i < (c_num_insertion / 2); i++)
    //        {
    //            table_parent_t parent = table_parent_t::get(table_parent_t::insert_row());
    //            gaia_id_t child = table_child_t::insert_row();
    //            parent.children().insert(child);
    //        }
    //        txn.commit();
    //    };
    //
    //    run_performance_test(insert5, "Simple relationships");

    //    auto insert6 = []() {
    //        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    //        for (int i = 0; i < (c_num_insertion / 2); i++)
    //        {
    //            table_parent_vlr_t::insert_row(i);
    //            table_child_vlr_t::insert_row(i);
    //        }
    //        txn.commit();
    //    };
    //
    //    run_performance_test(insert6, "Value Linked Relationships");

    for (size_t num_workers : {2, 5, 10})
    {
        auto insert7 = [num_workers]() {
            std::vector<std::thread> workers;

            for (size_t i = 0; i < num_workers; i++)
            {
                workers.emplace_back(insert_thread, (c_num_insertion / num_workers));
            }

            for (auto& worker : workers)
            {
                worker.join();
            }
        };
        run_performance_test(insert7, gaia_fmt::format("simple_table_t::insert_row with {} threads", num_workers), 10);
    }
}
