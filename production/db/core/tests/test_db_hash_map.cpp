/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/common/timer.hpp"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_hash_map.hpp"

using g_timer_t = gaia::common::timer_t;

using namespace gaia::db;
using namespace gaia::common;
using namespace std;

std::atomic<uint64_t> cnt = 0;

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class db_hash_map_test : public db_test_base_t
{
};

static const int64_t c_num_insertion = 100000;
static const int64_t c_max_insertion_single_txn = 50000;

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
    uint32_t num_iterations = 5,
    bool clear_db = true)
{
    accumulator_t<int64_t> expr_accumulator;

    for (uint32_t iteration = 0; iteration < num_iterations; iteration++)
    {
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);
    }

    log_performance_difference(expr_accumulator, message);
}

void insert_thread(int from)
{
    gaia::db::begin_session();
    gaia::db::begin_transaction();

    for (int i = 0; i < from; i++)
    {
        if (i > 0 && i % c_max_insertion_single_txn == 0)
        {
            gaia::db::commit_transaction();
            gaia::db::begin_transaction();
        }
        gaia_id_t id = cnt.fetch_add(1);
        auto hash_node = db_hash_map::insert(id);
        hash_node->locator = allocate_locator();
        gaia::db::allocate_object(hash_node->locator.load(), 10);
        db_object_t* obj_ptr = locator_to_ptr(hash_node->locator.load());

        if (!obj_ptr)
        {
            gaia_log::app().info("Botched id {} locator {}", id, hash_node->locator);
            throw gaia_exception("Botched id");
        }
        else
        {
            obj_ptr->id = id;
            gaia_locator_t locator = gaia::db::db_hash_map::find(obj_ptr->id);
            gaia_ptr_t ptr = gaia_ptr_t::from_locator(locator);
            ptr.finalize_create();
        }
    }

    if (gaia::db::is_transaction_open())
    {
        gaia::db::commit_transaction();
    }

    gaia::db::end_session();
}

TEST_F(db_hash_map_test, early_session_termination)
{
    // Test that closing the session after starting a transaction
    // does not generate any internal assertion failures
    // when attempting to reopen a session.

    size_t num_workers = 2;

    auto insert7 = [num_workers]() {
        std::vector<std::thread> workers;

        int num_insertions = c_num_insertion / num_workers;
        for (size_t i = 0; i < num_workers; i++)
        {
            workers.emplace_back(insert_thread, num_insertions);
        }

        for (auto& worker : workers)
        {
            worker.join();
        }
    };
    run_performance_test(insert7, gaia_fmt::format("db_hash_map::insert_row with {} threads", num_workers), 20);

    gaia_log::app().info("Inserted total: {}", cnt.load());

    begin_transaction();

    uint64_t not_found = 0;

    for (uint64_t i = 0; i < cnt.load(); i++)
    {
        if (db_hash_map::find(i) == c_invalid_gaia_locator)
        {
            not_found++;
            //            gaia_log::app().info("Locator not found for id: {}", i);
        }
    }

    gaia_log::app().info("Locator not found: {}", not_found);
    commit_transaction();
}
