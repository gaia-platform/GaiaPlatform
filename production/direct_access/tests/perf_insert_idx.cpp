////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_perf_idx.h"
#include "test_perf.hpp"

using namespace gaia::benchmark;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::perf_idx;
using namespace std;

class direct_access__perf_insert_idx__test : public gaia::db::db_catalog_test_base_t
{
public:
    direct_access__perf_insert_idx__test()
        : db_catalog_test_base_t("perf_idx.ddl", true, true, true)
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
    clear_table<unique_index_table_t>();
    clear_table<hash_index_table_t>();
    clear_table<range_index_table_t>();
}

TEST_F(direct_access__perf_insert_idx__test, unique_index_table)
{
    auto insert = []() {
        bulk_insert(&unique_index_table_t::insert_row);
    };

    run_performance_test(
        insert, clear_database, "unique_index_table_t::insert_row");
}

TEST_F(direct_access__perf_insert_idx__test, hash_index_table)
{
    auto insert = []() {
        bulk_insert(&hash_index_table_t::insert_row);
    };

    run_performance_test(
        insert, clear_database, "hash_index_table::insert_row");
}

TEST_F(direct_access__perf_insert_idx__test, range_index_table)
{
    auto insert = []() {
        bulk_insert(&range_index_table_t::insert_row);
    };

    run_performance_test(
        insert, clear_database, "range_index_table::insert_row");
}
