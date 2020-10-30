/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "gtest/gtest.h"

#include "catalog_tests_helper.hpp"
#include "db_test_base.hpp"
#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "type_id_record_id_cache_t.hpp"
#include "type_metadata.hpp"

using namespace gaia::catalog;
using namespace std;

/*
 * Utilities
 */

template <typename T_container>
size_t container_size(T_container container)
{
    size_t size{0};
    auto first = container.begin();
    auto last = container.end();

    for (; first != last; ++first)
    {
        ++size;
    }

    return size;
}

template <typename T_container>
gaia_field_t find_field(T_container fields, const std::string& field_name)
{
    auto it = std::find_if(fields.begin(), fields.end(), [&](const gaia_field_t& field) {
        return field_name == field.name();
    });

    if (it == fields.end())
    {
        throw gaia_exception(std::string("Expected field not found: ") + field_name);
    }

    return *it;
}

template <typename T_container>
gaia_relationship_t find_relationship(
    T_container relationships,
    const std::string& parent_table,
    const std::string& child_table,
    const std::string& child_field)
{
    auto it = std::find_if(relationships.begin(), relationships.end(), [&](gaia_relationship_t relationship) {
        return relationship.parent_gaia_table().name() == parent_table
            && relationship.child_gaia_table().name() == child_table
            && relationship.name() == child_field;
    });

    if (it == relationships.end())
    {
        stringstream message;
        message << "Expected relationship not found: " << parent_table << " --> " << child_table << "." << child_field;
        throw gaia_exception(message.str());
    }

    return *it;
}

/*
 * Tests
 */

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
    set<gaia_id_t> table_ids;
    constexpr int c_num_fields = 10;
    for (int i = 0; i < c_num_fields; i++)
    {
        table_ids.insert(create_table("list_tables_test_" + to_string(i), fields));
    }

    set<gaia_id_t> list_result;
    auto_transaction_t txn;
    {
        for (const auto& table : gaia_database_t::get(find_db_id("")).gaia_table_list())
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
        txn.commit();
    }
}

TEST_F(ddl_executor_test, drop_table_not_exist)
{
    string test_table_name{"a_not_existed_table"};
    EXPECT_THROW(drop_table(test_table_name), table_not_exists);
}

TEST_F(ddl_executor_test, drop_table_with_self_reference)
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
        txn.commit();
    }
}

TEST_F(ddl_executor_test, drop_table_parent_reference_fail)
{
    string parent_table_name{"parent_table"};
    ddl::field_def_list_t parent_fields;
    gaia_id_t parent_table_id = create_table(parent_table_name, parent_fields);

    string child_table_name{"child_table"};
    ddl::field_def_list_t child_fields;
    child_fields.emplace_back(make_unique<ddl::field_definition_t>("parent", data_type_t::e_references, 1, "parent_table"));
    create_table(child_table_name, child_fields);

    EXPECT_THROW(
        drop_table(parent_table_name),
        referential_integrity_violation);

    auto_transaction_t txn;
    auto table = gaia_table_t::get(parent_table_id);
    EXPECT_TRUE(table);
    ASSERT_EQ(1, container_size(table.parent_gaia_relationship_list()));
    txn.commit();
}

TEST_F(ddl_executor_test, drop_table_child_reference)
{
    string parent_table_name{"parent_table"};
    ddl::field_def_list_t parent_fields;
    gaia_id_t parent_table_id = create_table(parent_table_name, parent_fields);

    string child_table_name{"child_table"};
    ddl::field_def_list_t child_fields;
    child_fields.emplace_back(make_unique<ddl::field_definition_t>("parent", data_type_t::e_references, 1, "parent_table"));
    gaia_id_t child_table_id = create_table(child_table_name, child_fields);

    begin_transaction();
    gaia_table_t parent_table = gaia_table_t::get(parent_table_id);
    gaia_table_t child_table = gaia_table_t::get(child_table_id);
    commit_transaction();

    drop_table(child_table_name);

    begin_transaction();
    EXPECT_FALSE(gaia_table_t::get(child_table_id));

    // the relationship has been marked deprecated and the child has been unlinked.
    ASSERT_EQ(1, container_size(parent_table.parent_gaia_relationship_list()));
    gaia_relationship_t relationship = *parent_table.parent_gaia_relationship_list().begin();
    gaia_id_t relationship_id = relationship.gaia_id();

    ASSERT_TRUE(relationship.deprecated());
    EXPECT_FALSE(relationship.child_gaia_table());
    commit_transaction();

    drop_table(parent_table_name);

    begin_transaction();
    EXPECT_FALSE(gaia_table_t::get(parent_table_id));
    EXPECT_FALSE(gaia_relationship_t::get(relationship_id));
    commit_transaction();
}

TEST_F(ddl_executor_test, drop_database)
{
    string test_db_name{"drop_database_test"};
    gaia_id_t db_id = create_database(test_db_name);
    {
        auto_transaction_t txn;
        EXPECT_EQ(gaia_database_t::get(db_id).name(), test_db_name);
        txn.commit();
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
        txn.commit();
    }
}

TEST_F(ddl_executor_test, create_relationships)
{
    // (clinic) 1 --> N (doctor) 1 --> N (patient) N <-- 1 (clinic)

    gaia_id_t clinic_table_id
        = table_builder_t::new_table("clinic")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .field("location", data_type_t::e_string)
              .create();

    gaia_id_t doctor_table_id
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .field("surname", data_type_t::e_string)
              .reference("clinic", "hospital.clinic")
              .create();

    gaia_id_t patient_table_id
        = table_builder_t::new_table("patient")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .reference("doctor", "hospital.doctor")
              .reference("clinic", "hospital.clinic")
              .create();

    auto_transaction_t txn;
    gaia_table_t clinic_table = gaia_table_t::get(clinic_table_id);
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);
    gaia_table_t patient_table = gaia_table_t::get(patient_table_id);

    // Clinic appears twice as parent side of a relationship.
    ASSERT_EQ(2, container_size(clinic_table.parent_gaia_relationship_list()));

    // Doctor appears once as parent side of a relationship
    // and once as child side.
    ASSERT_EQ(1, container_size(doctor_table.parent_gaia_relationship_list()));
    ASSERT_EQ(1, container_size(doctor_table.child_gaia_relationship_list()));

    // Patient never appear as parent side of a relationship
    // and twice as child side.
    ASSERT_EQ(0, container_size(patient_table.parent_gaia_relationship_list()));
    ASSERT_EQ(2, container_size(patient_table.child_gaia_relationship_list()));

    // check clinic --> doctor

    // The relationship object must exists on both sides.
    gaia_relationship_t clinic_doctor_relationship = find_relationship(
        clinic_table.parent_gaia_relationship_list(), "clinic", "doctor", "clinic");

    gaia_relationship_t clinic_doctor_relationship2 = find_relationship(
        doctor_table.child_gaia_relationship_list(), "clinic", "doctor", "clinic");

    ASSERT_EQ(clinic_doctor_relationship.gaia_id(), clinic_doctor_relationship2.gaia_id());

    ASSERT_STREQ("clinic", clinic_doctor_relationship.name());
    ASSERT_EQ(uint8_t{0}, clinic_doctor_relationship.first_child_offset()); // clinic
    ASSERT_EQ(uint8_t{0}, clinic_doctor_relationship.next_child_offset()); // doctor
    ASSERT_EQ(uint8_t{1}, clinic_doctor_relationship.parent_offset()); // doctor

    // check doctor --> patient

    gaia_relationship_t doctor_patient_relationship = find_relationship(
        doctor_table.parent_gaia_relationship_list(), "doctor", "patient", "doctor");

    gaia_relationship_t doctor_patient_relationship2 = find_relationship(
        patient_table.child_gaia_relationship_list(), "doctor", "patient", "doctor");

    ASSERT_EQ(doctor_patient_relationship.gaia_id(), doctor_patient_relationship2.gaia_id());

    ASSERT_STREQ("doctor", doctor_patient_relationship.name());
    ASSERT_EQ(uint8_t{2}, doctor_patient_relationship.first_child_offset()); // doctor
    ASSERT_EQ(uint8_t{0}, doctor_patient_relationship.next_child_offset()); // patient
    ASSERT_EQ(uint8_t{1}, doctor_patient_relationship.parent_offset()); // patient

    // check clinic --> patient

    gaia_relationship_t clinic_patient_relationship = find_relationship(
        clinic_table.parent_gaia_relationship_list(), "clinic", "patient", "clinic");

    gaia_relationship_t clinic_patient_relationship2 = find_relationship(
        patient_table.child_gaia_relationship_list(), "clinic", "patient", "clinic");

    ASSERT_EQ(clinic_patient_relationship.gaia_id(), clinic_patient_relationship2.gaia_id());

    ASSERT_STREQ("clinic", clinic_patient_relationship.name());
    ASSERT_EQ(uint8_t{1}, clinic_patient_relationship.first_child_offset()); // clinic
    ASSERT_EQ(uint8_t{2}, clinic_patient_relationship.next_child_offset()); // patient
    ASSERT_EQ(uint8_t{3}, clinic_patient_relationship.parent_offset()); // patient
    txn.commit();
}

TEST_F(ddl_executor_test, create_anonymous_relationships)
{
    // (clinic) 1 -[anonymous]-> N (doctor)

    gaia_id_t clinic_table_id
        = table_builder_t::new_table("clinic")
              .database("hospital")
              .create();

    gaia_id_t doctor_table_id
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .reference("hospital.clinic")
              .create();

    auto_transaction_t txn;
    gaia_table_t clinic_table = gaia_table_t::get(clinic_table_id);
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);

    ASSERT_EQ(1, container_size(clinic_table.parent_gaia_relationship_list()));
    ASSERT_EQ(1, container_size(doctor_table.child_gaia_relationship_list()));
    txn.commit();
}

TEST_F(ddl_executor_test, create_self_relationships)
{
    // (doctor) 1 -[anonymous]-> N (doctor)

    gaia_id_t doctor_table_id
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .reference("self", "hospital.doctor")
              .create();

    auto_transaction_t txn;
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);

    ASSERT_EQ(1, container_size(doctor_table.parent_gaia_relationship_list()));
    ASSERT_EQ(1, container_size(doctor_table.child_gaia_relationship_list()));
    txn.commit();
}

TEST_F(ddl_executor_test, metadata)
{
    type_id_record_id_cache_t cache;
    // TODO this test should be in the SE, but since it depends on the Catalog we need to keep it here.

    // (clinic) 1 --> N (doctor) 1 --> N (patient) N <-- 1 (clinic)

    gaia_type_t clinic_type
        = table_builder_t::new_table("clinic")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .field("location", data_type_t::e_string)
              .create_type();

    gaia_type_t doctor_type
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .field("surname", data_type_t::e_string)
              .reference("clinic", "hospital.clinic")
              .create_type();

    gaia_type_t patient_type
        = table_builder_t::new_table("patient")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .reference("doctor", "hospital.doctor")
              .reference("clinic", "hospital.clinic")
              .create_type();

    auto_transaction_t txn;
    vector<gaia_id_t> types = {clinic_type, doctor_type, patient_type};
    for (gaia_id_t type_id : types)
    {
        gaia_id_t table_id = cache.get_record_id(type_id);
        gaia_table_t child_table = gaia_table_t::get(table_id);

        const type_metadata_t& metadata = type_registry_t::instance().get(child_table.type());

        for (gaia_relationship_t relationship : child_table.child_gaia_relationship_list())
        {
            auto relationship_metadata = metadata.find_child_relationship(relationship.parent_offset());

            ASSERT_TRUE(relationship_metadata.has_value());

            ASSERT_EQ(child_table.type(), relationship_metadata->child_type);
            ASSERT_EQ(relationship.parent_gaia_table().type(), relationship_metadata->parent_type);

            ASSERT_EQ(relationship.first_child_offset(), static_cast<uint8_t>(relationship_metadata->first_child_offset));
            ASSERT_EQ(relationship.next_child_offset(), static_cast<uint8_t>(relationship_metadata->next_child_offset));
            ASSERT_EQ(relationship.parent_offset(), static_cast<uint8_t>(relationship_metadata->parent_offset));
        }
    }
    txn.commit();
}

TEST_F(ddl_executor_test, metadata_not_exists)
{
    auto_transaction_t txn;

    const int c_non_existent_type = 1001;
    EXPECT_THROW(
        type_registry_t::instance().get(c_non_existent_type),
        invalid_type);

    txn.commit();
}
