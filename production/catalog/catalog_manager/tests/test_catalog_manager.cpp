/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <memory>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "flatbuffers/reflection.h"

#include "gaia_catalog.hpp"
#include "catalog_gaia_generated.h"
#include "fbs_generator.hpp"
#include "db_test_helpers.hpp"

using namespace gaia::catalog;
using namespace std;

class catalog_manager_test : public ::testing::Test {
  protected:
    void SetUp() override {
        gaia::db::begin_session();
    }

    void TearDown() override {
        gaia::db::end_session();
    }

    static void SetUpTestSuite() {
        gaia::db::start_server();
    }

    static void TearDownTestSuite() {
        gaia::db::stop_server();
    }

    static set<gaia_id_t> table_ids;

    gaia_id_t create_test_table(const string &name,
        const ddl::field_def_list_t &fields) {
        gaia_id_t table_id = create_table(name, fields);
        table_ids.insert(table_id);
        return table_id;
    }

    void check_table_name(gaia_id_t id, const string &name) {
        gaia::db::begin_transaction();
        Gaia_table t = Gaia_table::get(id);
        EXPECT_EQ(name, t.name());
        gaia::db::commit_transaction();
    }
};

set<gaia_id_t> catalog_manager_test::table_ids{};

TEST_F(catalog_manager_test, create_table) {
    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;

    gaia_id_t table_id = create_test_table(test_table_name, fields);

    check_table_name(table_id, test_table_name);
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

    EXPECT_EQ(table_ids, set<gaia_id_t>(list_tables().begin(), list_tables().end()));
}

TEST_F(catalog_manager_test, list_fields) {
    string test_table_name{"list_fields_test"};

    ddl::field_def_list_t test_table_fields;
    test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("id", data_type_t::INT8, 1)));
    test_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("name", data_type_t::STRING, 1)));

    gaia_id_t table_id = create_test_table(test_table_name, test_table_fields);

    EXPECT_EQ(test_table_fields.size(), list_fields(table_id).size());

    gaia::db::begin_transaction();
    uint16_t position = 0;
    for (gaia_id_t field_id : list_fields(table_id)) {
        Gaia_field field_record = Gaia_field::get(field_id);
        EXPECT_EQ(test_table_fields[position++]->name, field_record.name());
    }
    gaia::db::commit_transaction();
}

TEST_F(catalog_manager_test, list_references) {
    string dept_table_name{"list_references_test_department"};
    ddl::field_def_list_t dept_table_fields;
    dept_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("name", data_type_t::STRING, 1)));
    gaia_id_t dept_table_id = create_test_table(dept_table_name, dept_table_fields);

    string employee_table_name{"list_references_test_employee"};
    ddl::field_def_list_t employee_table_fields;
    employee_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("name", data_type_t::STRING, 1)));
    employee_table_fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("department", data_type_t::REFERENCES, 1, dept_table_name)));

    gaia_id_t employee_table_id = create_test_table(employee_table_name, employee_table_fields);

    EXPECT_EQ(list_fields(employee_table_id).size(), 1);
    EXPECT_EQ(list_references(employee_table_id).size(), 1);

    gaia::db::begin_transaction();
    gaia_id_t field_id = list_fields(employee_table_id).front();
    Gaia_field field_record = Gaia_field::get(field_id);
    EXPECT_EQ(employee_table_fields[0]->name, field_record.name());
    EXPECT_EQ(data_type_t::STRING, static_cast<data_type_t>(field_record.type()));
    EXPECT_EQ(1, field_record.position());

    gaia_id_t reference_id = list_references(employee_table_id).front();
    Gaia_field reference_record = Gaia_field::get(reference_id);
    EXPECT_EQ(employee_table_fields[1]->name, reference_record.name());
    EXPECT_EQ(data_type_t::REFERENCES, static_cast<data_type_t>(reference_record.type()));
    EXPECT_EQ(dept_table_id, reference_record.type_id());
    EXPECT_EQ(1, reference_record.position());
    gaia::db::commit_transaction();
}

TEST_F(catalog_manager_test, create_table_references_not_exist) {
    string test_table_name{"ref_not_exist_test"};
    ddl::field_def_list_t fields;
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("ref_field", data_type_t::REFERENCES, 1, "unknown")));
    EXPECT_THROW(create_test_table(test_table_name, fields), table_not_exists);
}

TEST_F(catalog_manager_test, create_table_self_references) {
    string test_table_name{"self_ref_table_test"};
    ddl::field_def_list_t fields;
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("self_ref_field", data_type_t::REFERENCES, 1, test_table_name)));

    gaia_id_t table_id = create_test_table(test_table_name, fields);
    gaia::db::begin_transaction();
    gaia_id_t reference_id = list_references(table_id).front();
    Gaia_field reference_record = Gaia_field::get(reference_id);
    EXPECT_EQ(fields.front()->name, reference_record.name());
    EXPECT_EQ(data_type_t::REFERENCES, static_cast<data_type_t>(reference_record.type()));
    EXPECT_EQ(table_id, reference_record.type_id());
    EXPECT_EQ(1, reference_record.position());
    gaia::db::commit_transaction();
}

TEST_F(catalog_manager_test, create_table_case_sensitivity) {
    string lower_case_table_name{"case_test_table"};
    string upper_case_table_name{"CASE_TEST_TABLE"};
    string mixed_case_table_name{"cAsE_teST_TablE"};
    ddl::field_def_list_t fields;

    gaia_id_t lower_case_table_id = create_test_table(lower_case_table_name, fields);
    gaia_id_t upper_case_table_id = create_test_table(upper_case_table_name, fields);
    gaia_id_t mixed_case_table_id = create_test_table(mixed_case_table_name, fields);

    check_table_name(lower_case_table_id, lower_case_table_name);
    check_table_name(upper_case_table_id, upper_case_table_name);
    check_table_name(mixed_case_table_id, mixed_case_table_name);

    string test_field_case_table_name{"test_field_case_table"};
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("field1", data_type_t::STRING, 1)));
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("Field1", data_type_t::STRING, 1)));
    gaia_id_t test_field_case_table_id = create_test_table(test_field_case_table_name, fields);
    check_table_name(test_field_case_table_id, test_field_case_table_name);
}

TEST_F(catalog_manager_test, create_table_duplicate_field) {
    string test_duplicate_field_table_name{"test_duplicate_field_table"};
    ddl::field_def_list_t fields;
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("field1", data_type_t::STRING, 1)));
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t("field1", data_type_t::STRING, 1)));
    EXPECT_THROW(create_test_table(test_duplicate_field_table_name, fields), duplicate_field);
}
