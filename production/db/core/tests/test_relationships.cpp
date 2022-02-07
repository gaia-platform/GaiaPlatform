/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "db_test_util.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::db::test;

class gaia_relationships_test : public db_test_base_t
{
protected:
    void SetUp() override
    {
        db_test_base_t::SetUp();
        gaia_id_t doctor_table_id = gaia::catalog::create_table("doctor", {});
        gaia_id_t patient_table_id = gaia::catalog::create_table("patient", {});

        begin_transaction();
        doctor_table_type = gaia::catalog::gaia_table_t::get(doctor_table_id).type();
        patient_table_type = gaia::catalog::gaia_table_t::get(patient_table_id).type();
        commit_transaction();
    }

    void TearDown() override
    {
        clean_type_registry();
        db_test_base_t::TearDown();
    }

    static gaia_type_t doctor_table_type;
    static gaia_type_t patient_table_type;
};

gaia_type_t gaia_relationships_test::doctor_table_type = c_invalid_gaia_type;
gaia_type_t gaia_relationships_test::patient_table_type = c_invalid_gaia_type;

// simone: I tried overloading the operator == with no success.
bool compare_relationships(const relationship_t& lhs, const relationship_t& rhs)
{
    return lhs.parent_type == rhs.parent_type
        && lhs.child_type == rhs.child_type
        && lhs.first_child_offset == rhs.first_child_offset
        && lhs.next_child_offset == rhs.next_child_offset
        && lhs.prev_child_offset == rhs.prev_child_offset
        && lhs.parent_offset == rhs.parent_offset
        && lhs.cardinality == rhs.cardinality
        && lhs.parent_required == rhs.parent_required
        && lhs.value_linked == rhs.value_linked;
}

TEST_F(gaia_relationships_test, metadata_init)
{
    const string db{"company"};
    const string employee_table{"employee"};
    const string address_table{"address"};

    gaia::catalog::create_database(db);
    gaia_id_t employee_table_id = gaia::catalog::create_table(db, employee_table, {});

    gaia::catalog::create_relationship(
        "company_manages",
        {"company", "employee", "reportees", "company", "employee", gaia::catalog::relationship_cardinality_t::many},
        {"company", "employee", "manager", "company", "employee", gaia::catalog::relationship_cardinality_t::one},
        false);

    // Expected pointers layout for employee type.
    const reference_offset_t c_employee_first_employee = 0;
    const reference_offset_t c_employee_parent_employee = 1;
    const reference_offset_t c_employee_next_child_employee = 2;
    const reference_offset_t c_employee_prev_child_employee = 3;
    const reference_offset_t c_employee_first_address = 4;

    gaia_id_t address_table_id = gaia::catalog::create_table(db, address_table, {});

    gaia::catalog::create_relationship(
        "company_address",
        {"company", "employee", "address", "company", "address", gaia::catalog::relationship_cardinality_t::one},
        {"company", "address", "owner", "company", "employee", gaia::catalog::relationship_cardinality_t::one},
        false);

    // Expected pointers layout for address type.
    const reference_offset_t c_address_parent_employee = 0;
    const reference_offset_t c_address_next_address = 1;

    begin_transaction();
    gaia_type_t employee_type = gaia::catalog::gaia_table_t::get(employee_table_id).type();
    gaia_type_t address_type = gaia::catalog::gaia_table_t::get(address_table_id).type();

    // type_registry_t::get() lazily initialize the metadata. It is important to NOT
    // call get() on address_type immediately to ensure that type_registry_t can
    // fetch both parent and child relationships without "touching" both types.
    const type_metadata_t& employee_meta = type_registry_t::instance().get(employee_type);

    // employee -[manages] -> employee
    optional<relationship_t> employee_employee_relationship1 = employee_meta.find_child_relationship(c_employee_parent_employee);
    ASSERT_TRUE(employee_employee_relationship1.has_value());
    ASSERT_EQ(c_employee_first_employee, employee_employee_relationship1->first_child_offset);
    ASSERT_EQ(c_employee_parent_employee, employee_employee_relationship1->parent_offset);
    ASSERT_EQ(c_employee_next_child_employee, employee_employee_relationship1->next_child_offset);
    ASSERT_EQ(c_employee_prev_child_employee, employee_employee_relationship1->prev_child_offset);

    optional<relationship_t> employee_employee_relationship2 = employee_meta.find_parent_relationship(c_employee_first_employee);
    ASSERT_TRUE(employee_employee_relationship2.has_value());
    ASSERT_EQ(c_employee_first_employee, employee_employee_relationship2->first_child_offset);
    ASSERT_EQ(c_employee_parent_employee, employee_employee_relationship2->parent_offset);
    ASSERT_EQ(c_employee_next_child_employee, employee_employee_relationship2->next_child_offset);
    ASSERT_EQ(c_employee_prev_child_employee, employee_employee_relationship2->prev_child_offset);

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

TEST_F(gaia_relationships_test, metadata_not_exists)
{
    begin_transaction();

    const int c_non_existent_type = 1001;
    EXPECT_THROW(
        type_registry_t::instance().get(c_non_existent_type),
        invalid_object_type);

    commit_transaction();
}

TEST_F(gaia_relationships_test, metadata_one_to_many)
{
    type_registry_t& test_registry = type_registry_t::instance();

    relationship_builder_t::one_to_many()
        .parent(doctor_table_type)
        .child(patient_table_type)
        .create_relationship();

    auto& parent = test_registry.get(doctor_table_type);
    auto& child = test_registry.get(patient_table_type);

    ASSERT_EQ(parent.get_type(), doctor_table_type);
    ASSERT_EQ(child.get_type(), patient_table_type);

    ASSERT_EQ(parent.references_count(), 1);
    ASSERT_EQ(child.references_count(), 3);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel.has_value());

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel.has_value());

    // Parent and child should be sharing the same relation.
    ASSERT_TRUE(compare_relationships(*parent_rel, *child_rel));

    ASSERT_EQ(parent_rel->parent_type, doctor_table_type);
    ASSERT_EQ(parent_rel->child_type, patient_table_type);
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::many);
}

TEST_F(gaia_relationships_test, metadata_one_to_one)
{
    type_registry_t& test_registry = type_registry_t::instance();

    relationship_builder_t::one_to_one()
        .parent(doctor_table_type)
        .child(patient_table_type)
        .create_relationship();

    auto& parent = test_registry.get(doctor_table_type);
    auto& child = test_registry.get(patient_table_type);

    ASSERT_EQ(parent.get_type(), doctor_table_type);
    ASSERT_EQ(child.get_type(), patient_table_type);

    ASSERT_EQ(parent.references_count(), 1);
    ASSERT_EQ(child.references_count(), 3);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel.has_value());

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel.has_value());

    // Parent and child should be sharing the same relation.
    ASSERT_TRUE(compare_relationships(*parent_rel, *child_rel));

    ASSERT_EQ(parent_rel->parent_type, doctor_table_type);
    ASSERT_EQ(parent_rel->child_type, patient_table_type);
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::one);
}

TEST_F(gaia_relationships_test, child_relation_do_not_use_next_child)
{
    type_registry_t& test_registry = type_registry_t::instance();

    relationship_builder_t::one_to_one()
        .parent(doctor_table_type)
        .child(patient_table_type)
        .create_relationship();

    auto& child = test_registry.get(patient_table_type);
    // although next_patient offset exists in child, it is not the one used
    // to identify the relation
    auto child_rel = child.find_child_relationship(c_next_patient_offset);

    ASSERT_FALSE(child_rel.has_value());
}
