/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "db_test_base.hpp"
#include "type_metadata.hpp"
#include "relations_test_util.h"

class gaia_se_references_test : public db_test_base_t {

    void TearDown() override {
        db_test_base_t::TearDown();
        type_registry_t::instance().clear();
    }
};

TEST_F(gaia_se_references_test, one_to_one) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr child = create_object(PATIENT_TYPE, "John Doe");

    parent.add_child_reference(child.id(), FIRST_PATIENT_OFFSET);

    ASSERT_EQ(parent.references()[FIRST_PATIENT_OFFSET], child.id());
    ASSERT_EQ(child.references()[PARENT_DOCTOR_OFFSET], parent.id());
    ASSERT_EQ(child.references()[NEXT_PATIENT_OFFSET], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, one_to_many) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr child = create_object(PATIENT_TYPE, "John Doe");
    gaia_ptr child2 = create_object(PATIENT_TYPE, "Jane Doe");

    parent.add_child_reference(child.id(), FIRST_PATIENT_OFFSET);
    parent.add_child_reference(child2.id(), FIRST_PATIENT_OFFSET);

    ASSERT_EQ(parent.references()[FIRST_PATIENT_OFFSET], child2.id());
    ASSERT_EQ(child.references()[PARENT_DOCTOR_OFFSET], parent.id());
    ASSERT_EQ(child.references()[NEXT_PATIENT_OFFSET], INVALID_GAIA_ID);
    ASSERT_EQ(child2.references()[PARENT_DOCTOR_OFFSET], parent.id());
    ASSERT_EQ(child2.references()[NEXT_PATIENT_OFFSET], child.id());

    commit_transaction();
}

TEST_F(gaia_se_references_test, single_cardinality_violation) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr child = create_object(PATIENT_TYPE, "John Doe");
    gaia_ptr child2 = create_object(PATIENT_TYPE, "Jane Doe");

    parent.add_child_reference(child.id(), FIRST_PATIENT_OFFSET);

    EXPECT_THROW(
        parent.add_child_reference(child2.id(), FIRST_PATIENT_OFFSET),
        single_cardinality_violation_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, invalid_relation_offset) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr child = create_object(PATIENT_TYPE, "John Doe");

    EXPECT_THROW(
        parent.add_child_reference(child.id(), NON_EXISTENT_OFFSET),
        invalid_relation_offset_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, invalid_relation_type_t_parent) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    gaia_type_t ADDRESS_TYPE = 101;
    relation_offset_t FIRST_ADDRESS_OFFSET = PARENT_DOCTOR_OFFSET + 1;
    relation_offset_t NEXT_ADDRESS_OFFSET = 0;
    relation_offset_t PARENT_PATIENT_OFFSET = 1;

    registry.get_metadata(ADDRESS_TYPE);

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    // need to create the relation to create the pointers in the payload
    // otherwise the invalid_relation_offset_t would be thrown.
    relation_builder_t::one_to_one()
        .parent(PATIENT_TYPE)
        .child(ADDRESS_TYPE)
        .first_child_offset(FIRST_ADDRESS_OFFSET)
        .next_child_offset(NEXT_ADDRESS_OFFSET)
        .parent_offset(PARENT_PATIENT_OFFSET)
        .add_to_registry(registry);

    gaia_ptr doctor = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr patient = create_object(PATIENT_TYPE, "Jane Doe");

    EXPECT_THROW(
        patient.add_child_reference(doctor.id(), FIRST_ADDRESS_OFFSET),
        invalid_relation_type_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, invalid_relation_type_t_child) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    gaia_type_t CLINIC_TYPE = 101;
    registry.get_metadata(CLINIC_TYPE);

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr clinic = create_object(CLINIC_TYPE, "Buena Vista Urgent Care");

    EXPECT_THROW(
        parent.add_child_reference(clinic.id(), FIRST_PATIENT_OFFSET),
        invalid_relation_type_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, child_already_in_relation_t) {
    begin_transaction();

    type_registry_t &registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(DOCTOR_TYPE, "Dr. House");
    gaia_ptr child = create_object(PATIENT_TYPE, "John Doe");

    parent.add_child_reference(child.id(), FIRST_PATIENT_OFFSET);
    EXPECT_THROW(
        parent.add_child_reference(child.id(), FIRST_PATIENT_OFFSET),
        child_already_in_relation_t);

    commit_transaction();
}