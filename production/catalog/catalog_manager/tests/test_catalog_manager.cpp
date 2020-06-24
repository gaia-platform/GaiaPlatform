/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "catalog_manager.hpp"
#include "catalog_gaia_generated.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

using namespace gaia::catalog;
using namespace std;

TEST(catalog_manager_test, create_table) {
    gaia::db::gaia_mem_base::init(true);
    string test_table_name{"test"};
    vector<ddl::field_definition_t *> fields;
    gaia_id_t table_id = catalog_manager_t::get().create_table(test_table_name, fields);

    gaia::db::begin_transaction();
    unique_ptr<Gaia_table> t{Gaia_table::get_row_by_id(table_id)};
    EXPECT_EQ(test_table_name, t->name());
    gaia::db::commit_transaction();
}
