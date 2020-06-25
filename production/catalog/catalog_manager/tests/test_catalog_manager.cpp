/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "catalog_gaia_generated.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>
#include <set>

using namespace gaia::catalog;
using namespace std;

class catalog_manager_test : public ::testing::Test {
  protected:
    static void SetUpTestSuite() {
        gaia::db::gaia_mem_base::init(true);
    }

    static set<gaia_id_t> table_ids;

    gaia_id_t create_test_table(const string &name,
        const vector<ddl::field_definition_t *> &fields) {
        gaia_id_t table_id = catalog_manager_t::get().create_table(name, fields);
        table_ids.insert(table_id);
        return table_id;
    }
};

set<gaia_id_t> catalog_manager_test::table_ids{};

TEST_F(catalog_manager_test, create_table) {
    string test_table_name{"create_table_test"};
    vector<ddl::field_definition_t *> fields;

    gaia_id_t table_id = create_test_table(test_table_name, fields);

    gaia::db::begin_transaction();
    unique_ptr<Gaia_table> t{Gaia_table::get_row_by_id(table_id)};
    EXPECT_EQ(test_table_name, t->name());
    gaia::db::commit_transaction();
}

TEST_F(catalog_manager_test, list_tables) {
    vector<ddl::field_definition_t *> fields;
    for (int i = 0; i < 10; i++) {
        create_test_table("list_tables_test_" + to_string(i), fields);
    }

    EXPECT_EQ(table_ids, set<gaia_id_t>(catalog_manager_t::get().list_tables().begin(),
                             catalog_manager_t::get().list_tables().end()));
}

TEST_F(catalog_manager_test, list_fields) {
    ddl::field_definition_t f1{"c1", ddl::data_type_t::INT8, 1};
    ddl::field_definition_t f2{"c2", ddl::data_type_t::STRING, 1};
    vector<ddl::field_definition_t *> fields{&f1, &f2};

    string test_table_name{"list_fields_test"};
    gaia_id_t table_id = catalog_manager_t::get().create_table(test_table_name, fields);
    table_ids.insert(table_id);

    EXPECT_EQ(fields.size(), catalog_manager_t::get().list_fields(table_id).size());

    gaia::db::begin_transaction();
    uint16_t position = 0;
    for (gaia_id_t field_id : catalog_manager_t::get().list_fields(table_id)) {
        unique_ptr<Gaia_field> field_record{Gaia_field::get_row_by_id(field_id)};
        EXPECT_EQ(fields[position++]->name, field_record->name());
    }
    gaia::db::commit_transaction();
}
