/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/catalog_tests_helper.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_addr_book.h"

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
    auto out_relationships = table.outgoing_relationships();

    auto it = std::find_if(
        out_relationships.begin(), out_relationships.end(),
        [&](gaia_relationship_t& relationship) {
            return relationship.to_child_link_name() == field_name;
        });

    if (it != out_relationships.end())
    {
        return *it;
    }

    auto in_relationships = table.incoming_relationships();

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
        : db_catalog_test_base_t("addr_book.ddl"){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        // These tests require a DDL session,
        // so we'll be closing the session opened in db_catalog_test_base_t::SetUp().
        end_session();
        begin_ddl_session();
    }

    void check_table_name(gaia_id_t id, const string& name)
    {
        gaia::db::begin_transaction();
        gaia_table_t t = gaia_table_t::get(id);
        EXPECT_EQ(name, t.name());
        gaia::db::commit_transaction();
    }

    string get_database_hash(const string& db_name)
    {
        gaia::db::begin_transaction();
        const auto& db_list = gaia_database_t::list().where(gaia_database_expr::name == db_name);
        EXPECT_TRUE(db_list.begin() != db_list.end());
        const char* database_hash = db_list.begin()->hash();
        gaia::db::commit_transaction();
        return string(database_hash);
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
    string test_db_name{"test_db"};
    create_database(test_db_name);

    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;

    gaia_id_t table_id = create_table(test_db_name, test_table_name, fields);
    check_table_name(table_id, test_table_name);

    ASSERT_THROW(create_table(test_db_name, test_table_name, fields), table_already_exists);
}

TEST_F(ddl_executor_test, system_tables)
{
    string test_table_name{"create_table_test"};
    ddl::field_def_list_t fields;
    gaia_id_t table_id = create_table(test_table_name, fields);

    auto_transaction_t txn;

    ASSERT_FALSE(gaia_table_t::get(table_id).is_system());

    for (gaia_table_t gaia_table : gaia_table_t::list())
    {
        if (strcmp(c_catalog_db_name.c_str(), gaia_table.database().name()) == 0)
        {
            ASSERT_TRUE(gaia_table.is_system());
        }
    }
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
    gaia_id_t empty_db_id = find_db_id(c_empty_db_name);
    {
        auto_transaction_t txn;
        for (const auto& table : gaia_database_t::get(empty_db_id).gaia_tables())
        {
            list_result.insert(table.gaia_id());
        }
        txn.commit();
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
        EXPECT_THROW(gaia_table_t::get(table_id), invalid_object_id);
        txn.commit();
    }
}

TEST_F(ddl_executor_test, drop_table_not_exist)
{
    string test_table_name{"a_not_existed_table"};

    EXPECT_NO_THROW(drop_table(test_table_name, false));

    EXPECT_THROW(drop_table(test_table_name), table_does_not_exist);
}

TEST_F(ddl_executor_test, drop_table_with_self_reference)
{
    string test_table_name{"self_ref_table"};
    ddl::field_def_list_t fields;
    gaia_id_t table_id = create_table(test_table_name, fields);
    check_table_name(table_id, test_table_name);

    gaia::catalog::create_relationship(
        test_table_name + "_self_ref_rel",
        {c_empty_db_name, test_table_name, "refs", c_empty_db_name, test_table_name, gaia::catalog::relationship_cardinality_t::many},
        {c_empty_db_name, test_table_name, "ref", c_empty_db_name, test_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    drop_table(test_table_name);
    {
        auto_transaction_t txn;
        EXPECT_THROW(gaia_table_t::get(table_id), invalid_object_id);
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
        {c_empty_db_name, parent_table_name, "children", c_empty_db_name, child_table_name, gaia::catalog::relationship_cardinality_t::many},
        {c_empty_db_name, child_table_name, "parent", c_empty_db_name, parent_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    EXPECT_THROW(
        drop_table(parent_table_name),
        referential_integrity_violation);

    auto_transaction_t txn;
    auto table = gaia_table_t::get(parent_table_id);
    EXPECT_TRUE(table);
    ASSERT_EQ(1, table.outgoing_relationships().size());
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
        {c_empty_db_name, parent_table_name, "children", c_empty_db_name, child_table_name, gaia::catalog::relationship_cardinality_t::many},
        {c_empty_db_name, child_table_name, "parent", c_empty_db_name, parent_table_name, gaia::catalog::relationship_cardinality_t::one},
        false);

    begin_transaction();
    gaia_table_t parent_table = gaia_table_t::get(parent_table_id);
    gaia_table_t child_table = gaia_table_t::get(child_table_id);
    commit_transaction();

    EXPECT_THROW(drop_table(child_table_name), referential_integrity_violation);
}

TEST_F(ddl_executor_test, drop_table_with_data)
{
    begin_transaction();
    auto w = gaia::addr_book::customer_writer();
    w.name = "Customer 1";
    w.insert_row();
    w.name = "Customer 2";
    w.insert_row();

    EXPECT_EQ(gaia::addr_book::customer_t::list().size(), 2);
    commit_transaction();

    ASSERT_NO_THROW(drop_table("addr_book", "customer", true));

    // After the table is dropped, users are not expected to use the direct
    // access API to access the table records. We still use the old direct
    // access API here only for testing purposes (to verify the data records are
    // indeed erased).
    // TODO: Switch to other methods for testing after GAIAPLAT-1623.
    begin_transaction();
    EXPECT_EQ(gaia::addr_book::customer_t::list().size(), 0);
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
        EXPECT_THROW(gaia_table_t::get(self_ref_table_id), invalid_object_id);
        EXPECT_THROW(gaia_table_t::get(test_table_id), invalid_object_id);
        EXPECT_THROW(gaia_database_t::get(db_id), invalid_object_id);
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
    ASSERT_EQ(2, clinic_table.outgoing_relationships().size());

    // Doctor appears once as parent side of a relationship
    // and once as child side.
    ASSERT_EQ(1, doctor_table.outgoing_relationships().size());
    ASSERT_EQ(1, doctor_table.incoming_relationships().size());

    // Patient never appear as parent side of a relationship
    // and twice as child side.
    ASSERT_EQ(0, patient_table.outgoing_relationships().size());
    ASSERT_EQ(2, patient_table.incoming_relationships().size());

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
    ASSERT_EQ(uint8_t{2}, clinic_to_doctor_relationship.prev_child_offset()); // doctor

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
    ASSERT_EQ(uint8_t{3}, doctor_to_patient_relationship.first_child_offset()); // doctor
    ASSERT_EQ(uint8_t{0}, doctor_to_patient_relationship.parent_offset()); // patient
    ASSERT_EQ(uint8_t{1}, doctor_to_patient_relationship.next_child_offset()); // patient
    ASSERT_EQ(uint8_t{2}, doctor_to_patient_relationship.prev_child_offset()); // patient

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
    ASSERT_EQ(uint8_t{3}, clinic_to_patient_relationship.parent_offset()); // patient
    ASSERT_EQ(uint8_t{4}, clinic_to_patient_relationship.next_child_offset()); // patient
    ASSERT_EQ(uint8_t{5}, clinic_to_patient_relationship.prev_child_offset()); // patient
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

    ASSERT_EQ(1, doctor_table.outgoing_relationships().size());
    ASSERT_EQ(1, doctor_table.incoming_relationships().size());

    gaia_relationship_t parent_relationship = *doctor_table.outgoing_relationships().begin();
    gaia_relationship_t child_relationship = *doctor_table.incoming_relationships().begin();

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

TEST_F(ddl_executor_test, create_index)
{
    string test_table_name{"create_index_test"};
    ddl::field_def_list_t test_table_fields;
    test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));

    gaia_id_t table_id = create_table(test_table_name, test_table_fields);

    check_table_name(table_id, test_table_name);
    string test_index_name{"test_index"};
    gaia_id_t index_id = create_index(test_index_name, true, index_type_t::hash, c_empty_db_name, test_table_name, {"name"});

    auto_transaction_t txn(false);
    ASSERT_STREQ(gaia_index_t::get(index_id).name(), test_index_name.c_str());
    ASSERT_EQ(gaia_index_t::get(index_id).table().gaia_id(), table_id);
    ASSERT_TRUE(gaia_table_t::get(table_id).gaia_fields().begin()->unique());
    txn.commit();

    ASSERT_THROW(
        create_index(test_index_name, true, index_type_t::hash, c_empty_db_name, test_table_name, {"name"}),
        index_already_exists);

    ASSERT_NO_THROW(create_index(test_index_name, true, index_type_t::hash, c_empty_db_name, test_table_name, {"name"}, false));
}

TEST_F(ddl_executor_test, create_index_duplicate_field)
{
    string test_table_name{"create_index_test"};
    ddl::field_def_list_t test_table_fields;
    test_table_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    gaia_id_t table_id = create_table(test_table_name, test_table_fields);
    check_table_name(table_id, test_table_name);

    string test_index_name{"test_index_dup"};
    ASSERT_THROW(
        create_index(test_index_name, true, index_type_t::hash, c_empty_db_name, test_table_name, {"name", "name"}),
        duplicate_field);
}

TEST_F(ddl_executor_test, list_indexes)
{
    // CREATE TABLE book(title STRING, author STRING, isbn STRING);
    // CREATE INDEX title_idx ON book(title);
    // CREATE INDEX author_idx ON book(author);
    // CREATE UNIQUE HASH INDEX isbn_idx ON book(isbn);
    gaia::catalog::ddl::field_def_list_t fields;
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("title", data_type_t::e_string, 1));
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("author", data_type_t::e_string, 1));
    fields.emplace_back(std::make_unique<gaia::catalog::ddl::data_field_def_t>("isbn", data_type_t::e_string, 1));
    auto table_id = gaia::catalog::create_table("book", fields);
    auto title_idx_id = gaia::catalog::create_index(
        "title_idx", false, gaia::catalog::index_type_t::range, c_empty_db_name, "book", {"title"});
    auto author_idx_id = gaia::catalog::create_index(
        "author_idx", false, gaia::catalog::index_type_t::range, c_empty_db_name, "book", {"author"});
    auto isbn_idx_id = gaia::catalog::create_index(
        "isbn_idx", true, gaia::catalog::index_type_t::hash, c_empty_db_name, "book", {"isbn"});

    std::set<gaia_id_t> index_ids;
    auto_transaction_t txn(false);
    for (const auto& idx : gaia_table_t::get(table_id).gaia_indexes())
    {
        index_ids.insert(idx.gaia_id());
    }

    ASSERT_NE(index_ids.find(title_idx_id), index_ids.end());
    ASSERT_NE(index_ids.find(author_idx_id), index_ids.end());
    ASSERT_NE(index_ids.find(isbn_idx_id), index_ids.end());

    vector<bool> unique_settings;
    transform(
        gaia_table_t::get(table_id).gaia_fields().begin(),
        gaia_table_t::get(table_id).gaia_fields().end(),
        back_inserter(unique_settings),
        [](const auto& field) -> bool { return field.unique(); });

    vector<bool> expected_unique_settings{true, false, false};
    ASSERT_EQ(unique_settings, expected_unique_settings);
}

TEST_F(ddl_executor_test, drop_relationship_with_data)
{
    begin_transaction();

    auto employee_writer = gaia::addr_book::employee_writer();
    employee_writer.name_first = "Michael";
    employee_writer.name_last = "Scott";
    gaia_id_t scott_id = employee_writer.insert_row();

    employee_writer.name_first = "Dwight";
    employee_writer.name_last = "Schrute";
    gaia_id_t schrute_id = employee_writer.insert_row();

    auto address_writer = gaia::addr_book::address_writer();
    address_writer.street = "Main Street";
    address_writer.city = "Honesdale";
    address_writer.state = "PA";
    address_writer.postal = "85364";
    address_writer.country = "United States";
    gaia_id_t schrute_farm_id = address_writer.insert_row();

    gaia::addr_book::employee_t::get(schrute_id).addresses().connect(schrute_farm_id);
    gaia::addr_book::employee_t::get(scott_id).reportees().connect(schrute_id);

    EXPECT_EQ(gaia::addr_book::employee_t::get(schrute_id).manager().gaia_id(), scott_id);
    EXPECT_EQ(gaia::addr_book::address_t::get(schrute_farm_id).owner().gaia_id(), schrute_id);

    commit_transaction();

    ASSERT_NO_THROW(drop_relationship("manager_reportees"));
    ASSERT_NO_THROW(drop_relationship("owner_addresses"));

    // After the relationship is dropped, users are not expected to use the
    // direct access API to access the links between tables. We still use the
    // old direct access API here only for testing purposes (to verify the links
    // are indeed erased).
    // TODO: Switch to other methods for testing after GAIAPLAT-1623.
    begin_transaction();

    EXPECT_EQ(gaia::addr_book::employee_t::get(schrute_id).manager().gaia_id(), c_invalid_gaia_id);
    EXPECT_EQ(gaia::addr_book::employee_t::get(scott_id).reportees().size(), 0);
    EXPECT_EQ(gaia::addr_book::employee_t::get(schrute_id).addresses().size(), 0);

    commit_transaction();
}

// Create a table, check resulting hash. Do same after dropping table. Expect euality.
TEST_F(ddl_executor_test, create_drop_create_hash)
{
    string test_table_name{"create_drop_create"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    create_table(test_table_name, fields);
    string first_hash = get_database_hash(c_empty_db_name);

    drop_table(test_table_name);

    // Add the table again and expect database hash to be the same.
    create_table(test_table_name, fields);
    string second_hash = get_database_hash(c_empty_db_name);
    EXPECT_STREQ(first_hash.c_str(), second_hash.c_str());
}

// Reverse table order in a named database. Expect equal hash results.
TEST_F(ddl_executor_test, reverse_table_order_hash)
{
    string test_db_name{"hash_db"};
    create_database(test_db_name);
    string first_test_table_name{"first_test_table"};
    string second_test_table_name{"second_test_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    create_table(test_db_name, first_test_table_name, fields);
    create_table(test_db_name, second_test_table_name, fields);
    string first_hash = get_database_hash(test_db_name);

    drop_table(test_db_name, first_test_table_name);
    drop_table(test_db_name, second_test_table_name);

    // Add the tables again in reverse order and expect database hash to be the same.
    create_table(test_db_name, second_test_table_name, fields);
    create_table(test_db_name, first_test_table_name, fields);
    string second_hash = get_database_hash(test_db_name);
    EXPECT_STREQ(first_hash.c_str(), second_hash.c_str());
}

// Reverse table order in an unnamed database. Expect equal hash results.
TEST_F(ddl_executor_test, empty_db_name_hash)
{
    string first_test_table_name{"first_test_table"};
    string second_test_table_name{"second_test_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    create_table(c_empty_db_name, first_test_table_name, fields);
    create_table(c_empty_db_name, second_test_table_name, fields);
    string first_hash = get_database_hash(c_empty_db_name);

    // The database with empty name must have a real hash.
    EXPECT_STRNE(first_hash.c_str(), "00000000000000000000000000000000");

    drop_table(first_test_table_name);
    drop_table(second_test_table_name);

    // Add the tables again in reverse order and expect database hash to be the same.
    create_table(c_empty_db_name, second_test_table_name, fields);
    create_table(c_empty_db_name, first_test_table_name, fields);
    string second_hash = get_database_hash(c_empty_db_name);
    EXPECT_STREQ(first_hash.c_str(), second_hash.c_str());
}

// Reverse field order in a table. Expect hash to be different.
TEST_F(ddl_executor_test, reverse_field_order_hash)
{
    string test_table_name{"field_order_table"};
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_string, 1));
    fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    create_table(test_table_name, fields);
    string first_hash = get_database_hash(c_empty_db_name);

    drop_table(test_table_name);

    // Add the table again with fields in reverse order and expect database hash to be different.
    ddl::field_def_list_t reverse_fields;
    reverse_fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
    reverse_fields.emplace_back(make_unique<data_field_def_t>("id", data_type_t::e_string, 1));
    create_table(test_table_name, reverse_fields);
    string second_hash = get_database_hash(c_empty_db_name);
    EXPECT_STRNE(first_hash.c_str(), second_hash.c_str());
}
