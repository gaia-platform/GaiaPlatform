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

static const int64_t c_num_insertion = 1000;
static const int64_t c_children_number = 10;

class test_insert_perf : public gaia::db::db_catalog_test_base_t
{
public:
    test_insert_perf()
        : db_catalog_test_base_t("insert.ddl"){};
};

template <typename T_type>
void clear_table()
{
    for (auto obj_it = T_type::list().begin();
         obj_it != T_type::list().end();)
    {
        auto next_obj_it = obj_it++;
        next_obj_it->delete_row();
    }
}

void clear_database()
{
    auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    clear_table<simple_table_t>();
    clear_table<simple_table_2_t>();
    clear_table<simple_table_3_t>();
    clear_table<simple_table_index_t>();
    clear_table<table_child_t>();
    clear_table<table_parent_t>();
    clear_table<table_child_vlr_t>();
    clear_table<table_parent_vlr_t>();
    txn.commit();
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
    uint32_t num_iterations = 5,
    bool clear_db = true)
{
    accumulator_t<int64_t> expr_accumulator;

    for (uint32_t iteration = 0; iteration < num_iterations; iteration++)
    {
        int64_t expr_duration = g_timer_t::get_function_duration(expr_fn);
        expr_accumulator.add(expr_duration);
        if (clear_db)
        {
            clear_database();
        }
    }

    log_performance_difference(expr_accumulator, message);
}

TEST_F(test_insert_perf, insert)
{
    sleep(1);
    cout << "Inserting " << c_num_insertion << " objects in the database for each test" << endl;

    auto insert1 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};

        for (int i = 0; i < c_num_insertion; i++)
        {
            simple_table_t::insert_row(i);
        }

        txn.commit();
    };

    run_performance_test(insert1, "simple_table_t::insert_row");

    auto insert2 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};

        for (int i = 0; i < c_num_insertion; i++)
        {
            simple_table_writer w;
            w.uint64_field = i;
            w.insert_row();
        }

        txn.commit();
    };

    run_performance_test(insert2, "simple_table_writer");

    auto insert3 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};

        for (int i = 0; i < c_num_insertion; i++)
        {
            simple_table_2_t::insert_row(i, "suppini", {1, 2, 3, 4, 5});
        }

        txn.commit();
    };

    run_performance_test(insert3, "simple_table_2_t::insert_row");

    auto insert4 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
        for (int i = 0; i < c_num_insertion; i++)
        {
            simple_table_index_t::insert_row(i);
        }
        txn.commit();
    };

    run_performance_test(insert4, "simple_table_index_t::insert_row");

    auto insert5 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
        for (int i = 0; i < (c_num_insertion / 2); i++)
        {
            table_parent_t parent = table_parent_t::get(table_parent_t::insert_row());
            gaia_id_t child = table_child_t::insert_row();
            parent.children().insert(child);
        }
        txn.commit();
    };

    run_performance_test(insert5, "Simple relationships");

    auto insert6 = []() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
        for (int i = 0; i < (c_num_insertion / 2); i++)
        {
            table_parent_vlr_t::insert_row(i);
            table_child_vlr_t::insert_row(i);
        }
        txn.commit();
    };

    run_performance_test(insert6, "Value Linked Relationships");
}

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/db/catalog_core.hpp"

#include "db_object_helpers.hpp"
#include "index_key.hpp"
#include "index_scan.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::db;
using namespace gaia::catalog;
using namespace gaia::db::query_processor::scan;
using namespace gaia::db::index;

TEST_F(test_insert_perf, query_single_match)
{

    auto_transaction_t txn{auto_transaction_t::no_auto_restart};
    for (int i = 0; i < 1000000; i++)
    {
        simple_table_index_t::insert_row(i);
    }

    // Lookup index_id for integer field.
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(simple_table_index_t::s_gaia_type);
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    for (const auto& index : catalog_core::list_indexes(type_record_id))
    {
        for (const auto& field_id : *index.fields())
        {
            const auto& field = catalog_core::field_view_t(gaia::db::id_to_ptr(field_id));

            if (field.data_type() == data_type_t::e_uint64 && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(hash_index_id != c_invalid_gaia_id);

    txn.commit();

    auto index = [&]() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
        uint64_t value = 10000;
        auto index_key = index_key_t(value);
        int num_results = 0;
        auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);
        for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
        {
            (void)scan;
            ++num_results;
        }

        txn.commit();

        gaia_log::app().info("Index res found: {}", num_results);
        EXPECT_EQ(num_results, 1);
    };

    run_performance_test(index, "Hash Index value found", 5, false);

    auto index2 = [&]() {
        auto_transaction_t txn{auto_transaction_t::no_auto_restart};
        uint64_t value = 1000000000;
        auto index_key = index_key_t(value);
        int num_results = 0;
        auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);
        for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
        {
            (void)scan;
            ++num_results;
        }

        txn.commit();

        gaia_log::app().info("Index res found: {}", num_results);
        EXPECT_EQ(num_results, 0);
    };

    run_performance_test(index2, "Hash Index value not found", 5, false);

    // Point-query on hash index.
}
