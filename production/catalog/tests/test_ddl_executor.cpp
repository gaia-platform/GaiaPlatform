/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "flatbuffers/reflection.h"
#include "gtest/gtest.h"

#include "db_test_base.hpp"
#include "fbs_generator.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"

using namespace gaia::catalog;
using namespace std;

class ddl_executor_test : public db_test_base_t
{
protected:
    void check_table_name(gaia_id_t id, const string& name)
    {
        gaia::db::begin_transaction();
        gaia_table_t t = gaia_table_t::get(id);
        EXPECT_EQ(name, t.name());
        gaia::db::commit_transaction();
    }
};

TEST_F(ddl_executor_test, create_database)
{
    string test_db_name{"create_database_test"};
    gaia_id_t db_id = create_database(test_db_name);
    begin_transaction();
    EXPECT_EQ(gaia_database_t::get(db_id).name(), test_db_name);
    commit_transaction();
}

TEST_F(ddl_executor_test, create_table)
{
    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;

    gaia_id_t table_id = create_table(test_table_name, fields);

    check_table_name(table_id, test_table_name);
}

TEST_F(ddl_executor_test, create_existing_table)
{
    string test_table_name{"create_existing_table"};
    ddl::field_def_list_t fields;

    create_table(test_table_name, fields);
    EXPECT_THROW(create_table(test_table_name, fields), table_already_exists);
}

TEST_F(ddl_executor_test, list_tables)
{
    ddl::field_def_list_t fields;
    const int count_tables = 10;
    set<gaia_id_t> table_ids;
    for (int i = 0; i < count_tables; i++)
    {
        table_ids.insert(create_table("list_tables_test_" + to_string(i), fields));
    }

    set<gaia_id_t> list_result;
    auto_transaction_t txn;
    {
        for (auto const& table : gaia_database_t::get(find_db_id("")).gaia_table_list())
        {
            list_result.insert(table.gaia_id());
        }
    }

    EXPECT_TRUE(includes(list_result.begin(), list_result.end(), table_ids.begin(), table_ids.end()));
}

TEST_F(ddl_executor_test, list_fields)
{
    string test_table_name{"list_fields_test"};

    ddl::field_def_list_t test_table_fields;
    test_table_fields.emplace_back(make_unique<ddl::field_definition_t>("id", data_type_t::e_int8, 1));
    test_table_fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    gaia::db::begin_transaction();
    EXPECT_EQ(test_table_fields.size(), list_fields(table_id).size());

    uint16_t position = 0;
    for (gaia_id_t field_id : list_fields(table_id))
    {
        gaia_field_t field_record = gaia_field_t::get(field_id);
        EXPECT_EQ(test_table_fields[position++]->name, field_record.name());
    }
    gaia::db::commit_transaction();
}

TEST_F(ddl_executor_test, list_references)
{
    string dept_table_name{"list_references_test_department"};
    ddl::field_def_list_t dept_table_fields;
    dept_table_fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    gaia_id_t dept_table_id = create_table(dept_table_name, dept_table_fields);

    string employee_table_name{"list_references_test_employee"};
    ddl::field_def_list_t employee_table_fields;
    employee_table_fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    employee_table_fields.emplace_back(
        make_unique<ddl::field_definition_t>("department", data_type_t::e_references, 1, dept_table_name));

    gaia_id_t employee_table_id = create_table(employee_table_name, employee_table_fields);

    gaia::db::begin_transaction();
    EXPECT_EQ(list_fields(employee_table_id).size(), 1);
    EXPECT_EQ(list_references(employee_table_id).size(), 1);

    gaia_id_t field_id = list_fields(employee_table_id).front();
    gaia_field_t field_record = gaia_field_t::get(field_id);
    EXPECT_EQ(employee_table_fields[0]->name, field_record.name());
    EXPECT_EQ(data_type_t::e_string, static_cast<data_type_t>(field_record.type()));
    EXPECT_EQ(0, field_record.position());

    gaia_id_t reference_id = list_references(employee_table_id).front();
    gaia_field_t reference_record = gaia_field_t::get(reference_id);
    EXPECT_EQ(employee_table_fields[1]->name, reference_record.name());
    EXPECT_EQ(data_type_t::e_references, static_cast<data_type_t>(reference_record.type()));
    EXPECT_EQ(dept_table_id, reference_record.ref_gaia_table().gaia_id());
    EXPECT_EQ(0, reference_record.position());
    gaia::db::commit_transaction();
}

TEST_F(ddl_executor_test, create_table_references_not_exist)
{
    string test_table_name{"ref_not_exist_test"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("ref_field", data_type_t::e_references, 1, "unknown"));
    EXPECT_THROW(create_table(test_table_name, fields), table_not_exists);
}

TEST_F(ddl_executor_test, create_table_self_references)
{
    string test_table_name{"self_ref_table_test"};
    ddl::field_def_list_t fields;
    fields.emplace_back(
        make_unique<ddl::field_definition_t>("self_ref_field", data_type_t::e_references, 1, test_table_name));

    gaia_id_t table_id = create_table(test_table_name, fields);
    gaia::db::begin_transaction();
    gaia_id_t reference_id = list_references(table_id).front();
    gaia_field_t reference_record = gaia_field_t::get(reference_id);
    EXPECT_EQ(fields.front()->name, reference_record.name());
    EXPECT_EQ(data_type_t::e_references, static_cast<data_type_t>(reference_record.type()));
    EXPECT_EQ(table_id, reference_record.ref_gaia_table().gaia_id());
    EXPECT_EQ(0, reference_record.position());
    gaia::db::commit_transaction();
}

TEST_F(ddl_executor_test, create_table_case_sensitivity)
{
    string lower_case_table_name{"case_test_table"};
    string upper_case_table_name{"CASE_TEST_TABLE"};
    string mixed_case_table_name{"cAsE_teST_TablE"};
    ddl::field_def_list_t fields;

    gaia_id_t lower_case_table_id = create_table(lower_case_table_name, fields);
    gaia_id_t upper_case_table_id = create_table(upper_case_table_name, fields);
    gaia_id_t mixed_case_table_id = create_table(mixed_case_table_name, fields);

    check_table_name(lower_case_table_id, lower_case_table_name);
    check_table_name(upper_case_table_id, upper_case_table_name);
    check_table_name(mixed_case_table_id, mixed_case_table_name);

    string test_field_case_table_name{"test_field_case_table"};
    fields.emplace_back(make_unique<ddl::field_definition_t>("field1", data_type_t::e_string, 1));
    fields.emplace_back(make_unique<ddl::field_definition_t>("Field1", data_type_t::e_string, 1));
    gaia_id_t test_field_case_table_id = create_table(test_field_case_table_name, fields);
    check_table_name(test_field_case_table_id, test_field_case_table_name);
}

TEST_F(ddl_executor_test, create_table_duplicate_field)
{
    string test_duplicate_field_table_name{"test_duplicate_field_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("field1", data_type_t::e_string, 1));
    fields.emplace_back(make_unique<ddl::field_definition_t>("field1", data_type_t::e_string, 1));
    EXPECT_THROW(create_table(test_duplicate_field_table_name, fields), duplicate_field);
}

TEST_F(ddl_executor_test, drop_table)
{
    string test_table_name{"drop_table_test"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    gaia_id_t table_id = create_table(test_table_name, fields);
    check_table_name(table_id, test_table_name);

    drop_table(test_table_name);
    {
        auto_transaction_t txn;
        auto table = gaia_table_t::get(table_id);
        EXPECT_FALSE(table);
    }
}

TEST_F(ddl_executor_test, drop_table_not_exist)
{
    string test_table_name{"a_not_existed_table"};
    EXPECT_THROW(drop_table(test_table_name), table_not_exists);
}

TEST_F(ddl_executor_test, drop_table_with_reference)
{
    string test_table_name{"self_ref_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("self_ref", data_type_t::e_references, 1));
    fields.back()->table_type_name = test_table_name;
    gaia_id_t table_id = create_table(test_table_name, fields);
    check_table_name(table_id, test_table_name);

    drop_table(test_table_name);
    {
        auto_transaction_t txn;
        auto table = gaia_table_t::get(table_id);
        EXPECT_FALSE(table);
    }
}

TEST_F(ddl_executor_test, drop_database)
{
    string test_db_name{"drop_database_test"};
    gaia_id_t db_id = create_database(test_db_name);
    {
        auto_transaction_t txn;
        EXPECT_EQ(gaia_database_t::get(db_id).name(), test_db_name);
    }

    string self_ref_table_name{"self_ref_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::field_definition_t>("self_ref", data_type_t::e_references, 1));
    fields.back()->table_type_name = self_ref_table_name;
    gaia_id_t self_ref_table_id = create_table(test_db_name, self_ref_table_name, fields);
    check_table_name(self_ref_table_id, self_ref_table_name);

    string test_table_name{"test_table"};
    fields.clear();
    fields.emplace_back(make_unique<ddl::field_definition_t>("name", data_type_t::e_string, 1));
    gaia_id_t test_table_id = create_table(test_db_name, test_table_name, fields);
    check_table_name(test_table_id, test_table_name);

    drop_database(test_db_name);
    {
        auto_transaction_t txn;
        EXPECT_FALSE(gaia_table_t::get(self_ref_table_id));
        EXPECT_FALSE(gaia_table_t::get(test_table_id));
        EXPECT_FALSE(gaia_database_t::get(db_id));
    }
}
