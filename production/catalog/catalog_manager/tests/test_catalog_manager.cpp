/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"
#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"
#include "gtest/gtest.h"
#include "flatbuffers/reflection.h"
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
        const ddl::field_def_list_t &fields) {
        gaia_id_t table_id = catalog_manager_t::get().create_table(name, fields);
        table_ids.insert(table_id);
        return table_id;
    }
};

set<gaia_id_t> catalog_manager_test::table_ids{};

TEST_F(catalog_manager_test, create_table) {
    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;

    gaia_id_t table_id = create_test_table(test_table_name, fields);

    gaia::db::begin_transaction();
    unique_ptr<Gaia_table> t{Gaia_table::get_row_by_id(table_id)};
    EXPECT_EQ(test_table_name, t->name());
    gaia::db::commit_transaction();
}

TEST_F(catalog_manager_test, create_existing_table) {
    string test_table_name{"create_existing_table"};
    ddl::field_def_list_t fields;

    create_test_table(test_table_name, fields);
    EXPECT_THROW(create_test_table(test_table_name, fields), table_already_exists);
}

TEST_F(catalog_manager_test, list_tables) {
    ddl::field_def_list_t fields;
    for (int i = 0; i < 10; i++) {
        create_test_table("list_tables_test_" + to_string(i), fields);
    }

    EXPECT_EQ(table_ids, set<gaia_id_t>(catalog_manager_t::get().list_tables().begin(),
                             catalog_manager_t::get().list_tables().end()));
}

TEST_F(catalog_manager_test, list_fields) {
    string test_table_name{"list_fields_test"};

    ddl::field_def_list_t test_table_fields;
    test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("id", ddl::data_type_t::INT8, 1)));
    test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("name", ddl::data_type_t::STRING, 1)));

    gaia_id_t table_id = create_test_table(test_table_name, test_table_fields);

    EXPECT_EQ(test_table_fields.size(), catalog_manager_t::get().list_fields(table_id).size());

    gaia::db::begin_transaction();
    uint16_t position = 0;
    for (gaia_id_t field_id : catalog_manager_t::get().list_fields(table_id)) {
        unique_ptr<Gaia_field> field_record{Gaia_field::get_row_by_id(field_id)};
        EXPECT_EQ(test_table_fields[position++]->name, field_record->name());
    }
    gaia::db::commit_transaction();
}
