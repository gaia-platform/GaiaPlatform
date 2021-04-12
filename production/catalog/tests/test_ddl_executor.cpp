/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include "gtest/gtest.h"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "catalog_tests_helper.hpp"
#include "type_id_mapping.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

/*
 * Utilities
 */

gaia_relationship_t find_relationship(
    const gaia_table_t& table,
    const std::string& field_name)
{
    auto out_relationships = table.gaia_relationships_parent();

    auto it = std::find_if(
        out_relationships.begin(), out_relationships.end(),
        [&](gaia_relationship_t& relationship) {
            return relationship.to_child_link_name() == field_name;
        });

    if (it != out_relationships.end())
    {
        return *it;
    }

    auto in_relationships = table.gaia_relationships_child();

    it = std::find_if(
        in_relationships.begin(), in_relationships.end(),
        [&](gaia_relationship_t& relationship) {
            return relationship.to_parent_link_name() == field_name;
        });

    if (it != out_relationships.end())
    {
        return *it;
    }

    stringstream message;
    message << "Expected relationship not found: " << table.name() << "." << field_name;
    throw gaia_exception(message.str());
}

/*
 * Tests
 */

class ddl_executor_test : public db_catalog_test_base_t
{
protected:
    ddl_executor_test()
        : db_catalog_test_base_t(){};

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
    string test_db_name{"create_database_test"};
    create_database(test_db_name);

    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;

    gaia_id_t table_id = create_table(test_table_name, fields);
    check_table_name(table_id, test_table_name);

    table_id = create_table(test_db_name, test_table_name, fields);
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
        for (const auto& table : gaia_database_t::get(find_db_id("")).gaia_tables())
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
    test_table_fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_int8, 1));
    test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));

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
    fields.emplace_back(make_unique<data_field_def_t>("field1", data_type_t::e_string, 1));
    fields.emplace_back(make_unique<data_field_def_t>("Field1", data_type_t::e_string, 1));
    gaia_id_t test_field_case_table_id = create_table(test_field_case_table_name, fields);
    check_table_name(test_field_case_table_id, test_field_case_table_name);
}

TEST_F(ddl_executor_test, create_table_duplicate_field)
{
    string test_duplicate_field_table_name{"test_duplicate_field_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("field1", data_type_t::e_string, 1));
    fields.emplace_back(make_unique<data_field_def_t>("field1", data_type_t::e_string, 1));
    EXPECT_THROW(create_table(test_duplicate_field_table_name, fields), duplicate_field);
}

TEST_F(ddl_executor_test, drop_table)
{
    string test_table_name{"drop_table_test"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
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
    gaia_id_t table_id = create_table(test_table_name, fields);
    check_table_name(table_id, test_table_name);

    gaia::catalog::create_relationship(
        test_table_name + "_self_ref_rel",
        {"", test_table_name, "refs", "", test_table_name, gaia::catalog::relationship_cardinality_t::many},
        {"", test_table_name, "ref", "", test_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

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
    create_table(child_table_name, child_fields);

    gaia::catalog::create_relationship(
        parent_table_name + child_table_name + "_test_rel1",
        {"", parent_table_name, "children", "", child_table_name, gaia::catalog::relationship_cardinality_t::many},
        {"", child_table_name, "parent", "", parent_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    EXPECT_THROW(
        drop_table(parent_table_name),
        referential_integrity_violation);

    auto_transaction_t txn;
    auto table = gaia_table_t::get(parent_table_id);
    EXPECT_TRUE(table);
    ASSERT_EQ(1, table.gaia_relationships_parent().size());
    txn.commit();
}

TEST_F(ddl_executor_test, drop_table_child_reference)
{
    string parent_table_name{"parent_table"};
    ddl::field_def_list_t parent_fields;
    gaia_id_t parent_table_id = create_table(parent_table_name, parent_fields);

    string child_table_name{"child_table"};
    ddl::field_def_list_t child_fields;
    gaia_id_t child_table_id = create_table(child_table_name, child_fields);

    gaia::catalog::create_relationship(
        parent_table_name + child_table_name + "_test_rel2",
        {"", parent_table_name, "children", "", child_table_name, gaia::catalog::relationship_cardinality_t::many},
        {"", child_table_name, "parent", "", parent_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    begin_transaction();
    gaia_table_t parent_table = gaia_table_t::get(parent_table_id);
    gaia_table_t child_table = gaia_table_t::get(child_table_id);
    commit_transaction();

    drop_table(child_table_name);

    begin_transaction();
    EXPECT_FALSE(gaia_table_t::get(child_table_id));

    // the relationship has been marked deprecated and the child has been unlinked.
    ASSERT_EQ(1, parent_table.gaia_relationships_parent().size());
    gaia_relationship_t relationship = *parent_table.gaia_relationships_parent().begin();
    gaia_id_t relationship_id = relationship.gaia_id();

    ASSERT_TRUE(relationship.deprecated());
    EXPECT_FALSE(relationship.child());
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
    gaia_id_t self_ref_table_id = create_table(test_db_name, self_ref_table_name, fields);
    check_table_name(self_ref_table_id, self_ref_table_name);

    gaia::catalog::create_relationship(
        test_db_name + "_self_ref_rel",
        {test_db_name, self_ref_table_name, "refs", test_db_name, self_ref_table_name, gaia::catalog::relationship_cardinality_t::many},
        {test_db_name, self_ref_table_name, "ref", test_db_name, self_ref_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    string test_table_name{"test_table"};
    fields.clear();
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
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
    // (clinic) 1 -[doctors]-> N (doctor)
    // (doctor) N -[clinic]-> 1 (clinic)
    //
    // (doctor) 1 -[patients]-> N (patient)
    // (patient) N -[doctor]-> 1 (doctor)
    //
    // (clinic) 1 -[patients]-> (patient)
    // (patient) N -[clinic]-> (clinic)

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
              .create();

    gaia_id_t patient_table_id
        = table_builder_t::new_table("patient")
              .database("hospital")
              .field("name", data_type_t::e_string)
              .create();

    gaia::catalog::create_relationship(
        "doctor_clinic",
        {"hospital", "clinic", "doctors", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "doctor", "clinic", "hospital", "clinic", gaia::catalog::relationship_cardinality_t::one},
        false);

    gaia::catalog::create_relationship(
        "patient_doctor",
        {"hospital", "doctor", "patients", "hospital", "patient", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "patient", "doctor", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::one},
        false);

    gaia::catalog::create_relationship(
        "patient_clinic",
        {"hospital", "clinic", "patients", "hospital", "patient", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "patient", "clinic", "hospital", "clinic", gaia::catalog::relationship_cardinality_t::one},
        false);

    auto_transaction_t txn;
    gaia_table_t clinic_table = gaia_table_t::get(clinic_table_id);
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);
    gaia_table_t patient_table = gaia_table_t::get(patient_table_id);

    // Clinic appears twice as parent side of a relationship.
    ASSERT_EQ(2, clinic_table.gaia_relationships_parent().size());

    // Doctor appears once as parent side of a relationship
    // and once as child side.
    ASSERT_EQ(1, doctor_table.gaia_relationships_parent().size());
    ASSERT_EQ(1, doctor_table.gaia_relationships_child().size());

    // Patient never appear as parent side of a relationship
    // and twice as child side.
    ASSERT_EQ(0, patient_table.gaia_relationships_parent().size());
    ASSERT_EQ(2, patient_table.gaia_relationships_child().size());

    // check
    // (clinic) 1 -[doctors]-> N (doctor)
    // (doctor) N -[clinic]-> 1 (clinic)

    // The relationship object must exists on both sides.
    gaia_relationship_t clinic_to_doctor_relationship = find_relationship(
        clinic_table, "doctors");

    gaia_relationship_t doctor_to_clinic_relationship = find_relationship(
        doctor_table, "clinic");

    ASSERT_EQ(clinic_to_doctor_relationship, doctor_to_clinic_relationship);

    ASSERT_STREQ("doctors", clinic_to_doctor_relationship.to_child_link_name());
    ASSERT_STREQ("clinic", clinic_to_doctor_relationship.to_parent_link_name());
    ASSERT_EQ(uint8_t{0}, clinic_to_doctor_relationship.first_child_offset()); // clinic
    ASSERT_EQ(uint8_t{0}, clinic_to_doctor_relationship.parent_offset()); // doctor
    ASSERT_EQ(uint8_t{1}, clinic_to_doctor_relationship.next_child_offset()); // doctor

    // check
    // (doctor) 1 -[patients]-> N (patient)
    // (patient) N -[doctor]-> 1 (doctor)

    gaia_relationship_t doctor_to_patient_relationship = find_relationship(
        doctor_table, "patients");

    gaia_relationship_t patient_to_doctor_relationship = find_relationship(
        patient_table, "doctor");

    ASSERT_EQ(doctor_to_patient_relationship, patient_to_doctor_relationship);

    ASSERT_STREQ("patients", doctor_to_patient_relationship.to_child_link_name());
    ASSERT_STREQ("doctor", doctor_to_patient_relationship.to_parent_link_name());
    ASSERT_EQ(uint8_t{2}, doctor_to_patient_relationship.first_child_offset()); // doctor
    ASSERT_EQ(uint8_t{0}, doctor_to_patient_relationship.parent_offset()); // patient
    ASSERT_EQ(uint8_t{1}, doctor_to_patient_relationship.next_child_offset()); // patient

    // check
    // (clinic) 1 -[patients]-> (patient)
    // (patient) N -[clinic]-> (clinic)

    gaia_relationship_t clinic_to_patient_relationship = find_relationship(
        clinic_table, "patients");

    gaia_relationship_t patient_to_clinic_relationship = find_relationship(
        patient_table, "clinic");

    ASSERT_EQ(clinic_to_patient_relationship, patient_to_clinic_relationship);

    ASSERT_STREQ("patients", clinic_to_patient_relationship.to_child_link_name());
    ASSERT_STREQ("clinic", clinic_to_patient_relationship.to_parent_link_name());
    ASSERT_EQ(uint8_t{1}, clinic_to_patient_relationship.first_child_offset()); // clinic
    ASSERT_EQ(uint8_t{2}, clinic_to_patient_relationship.parent_offset()); // patient
    ASSERT_EQ(uint8_t{3}, clinic_to_patient_relationship.next_child_offset()); // patient
    txn.commit();
}

TEST_F(ddl_executor_test, create_self_relationships)
{
    // (doctor) 1 -[doctor]-> N (doctor)
    // (doctor) N -[doctors]-> 1 (doctor)

    gaia_id_t doctor_table_id
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .create();

    gaia::catalog::create_relationship(
        "doctor_doctor",
        {"hospital", "doctor", "doctors", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "doctor", "doctor", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::one},
        false);

    auto_transaction_t txn;
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);

    ASSERT_EQ(1, doctor_table.gaia_relationships_parent().size());
    ASSERT_EQ(1, doctor_table.gaia_relationships_child().size());

    gaia_relationship_t parent_relationship = *doctor_table.gaia_relationships_parent().begin();
    gaia_relationship_t child_relationship = *doctor_table.gaia_relationships_child().begin();

    ASSERT_EQ(parent_relationship, child_relationship);

    ASSERT_STREQ("doctors", child_relationship.to_child_link_name());
    ASSERT_STREQ("doctor", child_relationship.to_parent_link_name());

    ASSERT_EQ(doctor_table, parent_relationship.parent());
    ASSERT_EQ(doctor_table, parent_relationship.child());

    ASSERT_EQ(uint8_t{0}, parent_relationship.first_child_offset()); // clinic
    ASSERT_EQ(uint8_t{1}, parent_relationship.parent_offset()); // patient
    ASSERT_EQ(uint8_t{2}, parent_relationship.next_child_offset()); // patient

    txn.commit();
}

TEST_F(ddl_executor_test, multiple_relationship_same_table)
{

    gaia_id_t doctor_table_id
        = table_builder_t::new_table("doctor")
              .database("hospital")
              .create();

    gaia_id_t patient_table_id
        = table_builder_t::new_table("patient")
              .database("hospital")
              .create();

    gaia::catalog::create_relationship(
        "patient_current_doctor",
        {"hospital", "doctor", "current_patients", "hospital", "patient", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "patient", "current_doctor", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::one},
        false);

    gaia::catalog::create_relationship(
        "patient_current_doctor",
        {"hospital", "doctor", "past_patients", "hospital", "patient", gaia::catalog::relationship_cardinality_t::many},
        {"hospital", "patient", "past_doctor", "hospital", "doctor", gaia::catalog::relationship_cardinality_t::one},
        false);

    auto_transaction_t txn;
    gaia_table_t doctor_table = gaia_table_t::get(doctor_table_id);
    gaia_table_t patient_table = gaia_table_t::get(patient_table_id);

    ASSERT_EQ(2, doctor_table.gaia_relationships_parent().size());
    ASSERT_EQ(0, doctor_table.gaia_relationships_child().size());
    ASSERT_EQ(0, patient_table.gaia_relationships_parent().size());
    ASSERT_EQ(2, patient_table.gaia_relationships_child().size());

    gaia_relationship_t current_doctor_relationship = find_relationship(patient_table, "current_doctor");
    gaia_relationship_t past_doctor_relationship = find_relationship(patient_table, "past_doctor");

    ASSERT_STREQ("current_patients", current_doctor_relationship.to_child_link_name());
    ASSERT_STREQ("current_doctor", current_doctor_relationship.to_parent_link_name());
    ASSERT_STREQ("past_patients", past_doctor_relationship.to_child_link_name());
    ASSERT_STREQ("past_doctor", past_doctor_relationship.to_parent_link_name());

    ASSERT_EQ(doctor_table, current_doctor_relationship.parent());
    ASSERT_EQ(patient_table, current_doctor_relationship.child());
    ASSERT_EQ(doctor_table, past_doctor_relationship.parent());
    ASSERT_EQ(patient_table, past_doctor_relationship.child());

    ASSERT_EQ(uint8_t{0}, current_doctor_relationship.first_child_offset());
    ASSERT_EQ(uint8_t{0}, current_doctor_relationship.parent_offset());
    ASSERT_EQ(uint8_t{1}, current_doctor_relationship.next_child_offset());
    ASSERT_EQ(uint8_t{1}, past_doctor_relationship.first_child_offset());
    ASSERT_EQ(uint8_t{2}, past_doctor_relationship.parent_offset());
    ASSERT_EQ(uint8_t{3}, past_doctor_relationship.next_child_offset());

    txn.commit();
}

TEST_F(ddl_executor_test, metadata_init)
{
    // TODO this test should be in the SE, but since it depends on the Catalog we need to keep it here.

    gaia_type_t employee_type
        = table_builder_t::new_table("employee")
              // Can't use `addr_book` database name (as in other tests) because it will to conflict
              // with the database created by other tests that do not properly clean up the data.
              // (same below)
              .database("company")
              .create_type();

    gaia::catalog::create_relationship(
        "company_manages",
        {"company", "employee", "reportees", "company", "employee", gaia::catalog::relationship_cardinality_t::many},
        {"company", "employee", "manager", "company", "employee", gaia::catalog::relationship_cardinality_t::one},
        false);

    // Expected pointers layout for employee type.
    const reference_offset_t c_employee_first_employee = 0;
    const reference_offset_t c_employee_parent_employee = 1;
    const reference_offset_t c_employee_child_employee = 2;
    const reference_offset_t c_employee_first_address = 3;

    gaia_type_t address_type
        = table_builder_t::new_table("address")
              .database("company")
              .create_type();

    gaia::catalog::create_relationship(
        "company_address",
        {"company", "employee", "address", "company", "address", gaia::catalog::relationship_cardinality_t::one},
        {"company", "address", "owner", "company", "employee", gaia::catalog::relationship_cardinality_t::one},
        false);

    // Expected pointers layout for address type.
    const reference_offset_t c_address_parent_employee = 0;
    const reference_offset_t c_address_next_address = 1;

    begin_transaction();

    // type_registry_t::get() lazily initialize the metadata. It is important to NOT
    // call get() on address_type immediately to ensure that type_registry_t can
    // fetch both parent and child relationships without "touching" both types.
    const type_metadata_t& employee_meta = type_registry_t::instance().get(employee_type);

    // employee -[manages] -> employee
    optional<relationship_t> employee_employee_relationship1 = employee_meta.find_child_relationship(c_employee_parent_employee);
    ASSERT_TRUE(employee_employee_relationship1.has_value());
    ASSERT_EQ(c_employee_first_employee, employee_employee_relationship1->first_child_offset);
    ASSERT_EQ(c_employee_parent_employee, employee_employee_relationship1->parent_offset);
    ASSERT_EQ(c_employee_child_employee, employee_employee_relationship1->next_child_offset);

    optional<relationship_t> employee_employee_relationship2 = employee_meta.find_parent_relationship(c_employee_first_employee);
    ASSERT_TRUE(employee_employee_relationship2.has_value());
    ASSERT_EQ(c_employee_first_employee, employee_employee_relationship2->first_child_offset);
    ASSERT_EQ(c_employee_parent_employee, employee_employee_relationship2->parent_offset);
    ASSERT_EQ(c_employee_child_employee, employee_employee_relationship2->next_child_offset);

    // employee -[address]-> address
    optional<relationship_t> employee_address_relationship1 = employee_meta.find_parent_relationship(c_employee_first_address);
    ASSERT_TRUE(employee_address_relationship1.has_value());
    ASSERT_EQ(c_employee_first_address, employee_address_relationship1->first_child_offset);
    ASSERT_EQ(c_address_parent_employee, employee_address_relationship1->parent_offset);
    ASSERT_EQ(c_address_next_address, employee_address_relationship1->next_child_offset);

    const type_metadata_t& address_meta = type_registry_t::instance().get(address_type);

    // employee -[address]-> address
    optional<relationship_t> employee_address_relationship2 = address_meta.find_child_relationship(c_address_parent_employee);
    ASSERT_TRUE(employee_address_relationship2.has_value());
    ASSERT_EQ(c_employee_first_address, employee_address_relationship2->first_child_offset);
    ASSERT_EQ(c_address_parent_employee, employee_address_relationship2->parent_offset);
    ASSERT_EQ(c_address_next_address, employee_address_relationship2->next_child_offset);

    commit_transaction();
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
