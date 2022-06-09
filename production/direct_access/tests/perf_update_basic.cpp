////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_basic.h"
#include "test_perf.hpp"

using namespace gaia::benchmark;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::perf_basic;
using namespace std;

class direct_access__perf_update_basic__test : public gaia::db::db_catalog_test_base_t
{
public:
    direct_access__perf_update_basic__test()
        : db_catalog_test_base_t("perf_basic.ddl", true, true, true)
    {
    }

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
    clear_table<simple_table_t>();
    clear_table<simple_table_2_t>();
    clear_table<simple_table_3_t>();
}

void insert_data()
{
    gaia::db::begin_transaction();

    for (size_t i = 0; i < c_num_records; i++)
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
}

TEST_F(direct_access__perf_update_basic__test, simple_table_update)
{
    insert_data();

    auto update = []() {
        bulk_update<simple_table_t>([](simple_table_t& obj) {
            simple_table_writer w = obj.writer();
            w.uint64_field = 1;
            w.update_row();
        });
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        update, clear_database, "simple_table_update", clear_db_after_each_iteration);
}

TEST_F(direct_access__perf_update_basic__test, simple_table_update_dynamic)
{
    insert_data();

    auto update = []() {
        bulk_update<simple_table_t>([](simple_table_t& obj) {
            simple_table_writer w = obj.writer();
            w.uint64_field = obj.uint64_field() + 1;
            w.update_row();
        });
    };

    bool clear_db_after_each_iteration = false;
    run_performance_test(
        update, clear_database, "simple_table_update_dynamic", clear_db_after_each_iteration);
}
