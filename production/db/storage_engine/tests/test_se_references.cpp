/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"
#include "db_test_base.hpp"
#include "type_metadata.hpp"
#include "relations_test_util.h"

using namespace gaia::db::test;

class gaia_se_references_test : public db_test_base_t {
    void TearDown() override {
        db_test_base_t::TearDown();
        clean_type_registry();
    }
};

// The add_parent_reference API ends up calling the add_child API. What
// is covered by the add_child API is not tested again in add_parent.

TEST_F(gaia_se_references_test, add_child_reference__one_to_one) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);

    ASSERT_EQ(parent.references()[c_first_patient_offset], child.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], parent.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__one_to_many) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");
    gaia_ptr child2 = create_object(c_patient_type, "Jane Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);
    parent.add_child_reference(child2.id(), c_first_patient_offset);

    ASSERT_EQ(parent.references()[c_first_patient_offset], child2.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], parent.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);
    ASSERT_EQ(child2.references()[c_parent_doctor_offset], parent.id());
    ASSERT_EQ(child2.references()[c_next_patient_offset], child.id());

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__single_cardinality_violation) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");
    gaia_ptr child2 = create_object(c_patient_type, "Jane Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);

    EXPECT_THROW(
        parent.add_child_reference(child2.id(), c_first_patient_offset),
        single_cardinality_violation_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__invalid_relation_offset) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    EXPECT_THROW(
        parent.add_child_reference(child.id(), c_non_existent_offset),
        invalid_relation_offset_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__invalid_relation_type_parent) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    gaia_type_t ADDRESS_TYPE = 101;
    relationship_offset_t FIRST_ADDRESS_OFFSET = c_parent_doctor_offset + 1;
    relationship_offset_t NEXT_ADDRESS_OFFSET = 0;
    relationship_offset_t PARENT_PATIENT_OFFSET = 1;

    registry.get_or_create(ADDRESS_TYPE);

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    // Need to create the relation to create the pointers in the payload
    // otherwise the invalid_relation_offset_t would be thrown.
    relation_builder_t::one_to_one()
        .parent(c_patient_type)
        .child(ADDRESS_TYPE)
        .first_child_offset(FIRST_ADDRESS_OFFSET)
        .next_child_offset(NEXT_ADDRESS_OFFSET)
        .parent_offset(PARENT_PATIENT_OFFSET)
        .add_to_registry(registry);

    gaia_ptr doctor = create_object(c_doctor_type, "Dr. House");
    gaia_ptr patient = create_object(c_patient_type, "Jane Doe");

    EXPECT_THROW(
        patient.add_child_reference(doctor.id(), FIRST_ADDRESS_OFFSET),
        invalid_relation_type_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__invalid_relation_type_child) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    gaia_type_t CLINIC_TYPE = 101;
    registry.get_or_create(CLINIC_TYPE);

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr clinic = create_object(CLINIC_TYPE, "Buena Vista Urgent Care");

    EXPECT_THROW(
        parent.add_child_reference(clinic.id(), c_first_patient_offset),
        invalid_relation_type_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__child_already_in_relation) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);
    EXPECT_THROW(
        parent.add_child_reference(child.id(), c_first_patient_offset),
        child_already_in_relation_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_parent_reference__one_to_many) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    child.add_parent_reference(parent, c_parent_doctor_offset);

    ASSERT_EQ(parent.references()[c_first_patient_offset], child.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], parent.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_parent_reference__fail_on_wrong_offset) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    EXPECT_THROW(
        child.add_parent_reference(parent, c_next_patient_offset),
        invalid_relation_offset_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_child_reference__invalid_node_id) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");

    EXPECT_THROW(
        parent.add_child_reference(c_non_existent_id, c_next_patient_offset),
        invalid_node_id);

    commit_transaction();
}

TEST_F(gaia_se_references_test, add_parent_reference__invalid_node_id) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr child = create_object(c_patient_type, "John Doe");

    EXPECT_THROW(
        child.add_parent_reference(c_non_existent_id, c_parent_doctor_offset),
        invalid_node_id);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__one_to_one) {
    begin_transaction();

    // GIVEN

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);

    // WHEN

    parent.remove_child_reference(child.id(), c_first_patient_offset);

    // THEN

    ASSERT_EQ(parent.references()[c_first_patient_offset], INVALID_GAIA_ID);
    ASSERT_EQ(child.references()[c_parent_doctor_offset], INVALID_GAIA_ID);
    ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__many_to_many_from_back) {
    begin_transaction();

    // GIVEN

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");

    vector<gaia_ptr> children;

    for (auto patient_name : {"Jhon Doe", "Jane Doe", "Foo", "Bar"}) {
        gaia_ptr child = create_object(c_patient_type, patient_name);
        parent.add_child_reference(child.id(), c_first_patient_offset);
        children.push_back(child);
    }

    // add_child_reference adds to the head
    for (size_t i = children.size() - 1; i < children.size(); --i) {
        auto child = children[i];

        // WHEN
        parent.remove_child_reference(child.id(), c_first_patient_offset);

        // THEN
        if (i > 0) {
            ASSERT_EQ(parent.references()[c_first_patient_offset], children[i - 1].id());
        }
        ASSERT_EQ(child.references()[c_parent_doctor_offset], INVALID_GAIA_ID);
        ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);
    }

    // when the last child is removed the parent needs to be updated as well.
    ASSERT_EQ(parent.references()[c_first_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__many_to_many_from_head) {
    begin_transaction();

    // GIVEN

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");

    vector<gaia_ptr> children;

    for (auto patient_name : {"Jhon Doe", "Jane Doe", "Foo", "Bar"}) {
        gaia_ptr child = create_object(c_patient_type, patient_name);
        parent.add_child_reference(child.id(), c_first_patient_offset);
        children.push_back(child);
    }

    // add_child_reference adds to the head
    for (const auto& child : children) {
        // WHEN
        parent.remove_child_reference(child.id(), c_first_patient_offset);

        // THEN
        if (child.id() != children.back().id()) {
            ASSERT_EQ(parent.references()[c_first_patient_offset], children.back().id());
        }
        ASSERT_EQ(child.references()[c_parent_doctor_offset], INVALID_GAIA_ID);
        ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);
    }

    // when the last child is removed the parent needs to be updated as well.
    ASSERT_EQ(parent.references()[c_first_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__invalid_relation_offset) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    EXPECT_THROW(
        parent.remove_child_reference(child.id(), c_non_existent_offset),
        invalid_relation_offset_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__invalid_node_id) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");

    EXPECT_THROW(
        parent.remove_child_reference(c_non_existent_id, c_next_patient_offset),
        invalid_node_id);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__invalid_relation_type_parent) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    gaia_type_t ADDRESS_TYPE = 101;
    relationship_offset_t FIRST_ADDRESS_OFFSET = c_parent_doctor_offset + 1;
    relationship_offset_t NEXT_ADDRESS_OFFSET = 0;
    relationship_offset_t PARENT_PATIENT_OFFSET = 1;

    registry.get_or_create(ADDRESS_TYPE);

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    // Need to create the relation to create the pointers in the payload
    // otherwise the invalid_relation_offset_t would be thrown.
    relation_builder_t::one_to_one()
        .parent(c_patient_type)
        .child(ADDRESS_TYPE)
        .first_child_offset(FIRST_ADDRESS_OFFSET)
        .next_child_offset(NEXT_ADDRESS_OFFSET)
        .parent_offset(PARENT_PATIENT_OFFSET)
        .add_to_registry(registry);

    gaia_ptr doctor = create_object(c_doctor_type, "Dr. House");
    gaia_ptr patient = create_object(c_patient_type, "Jane Doe");

    EXPECT_THROW(
        patient.remove_child_reference(doctor.id(), FIRST_ADDRESS_OFFSET),
        invalid_relation_type_t);

    commit_transaction();
}

TEST_F(gaia_se_references_test, remove_child_reference__invalid_relation_type_child) {
    begin_transaction();

    type_registry_t& registry = type_registry_t::instance();

    gaia_type_t CLINIC_TYPE = 101;
    registry.get_or_create(CLINIC_TYPE);

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr clinic = create_object(CLINIC_TYPE, "Buena Vista Urgent Care");

    EXPECT_THROW(
        parent.remove_child_reference(clinic.id(), c_first_patient_offset),
        invalid_relation_type_t);

    commit_transaction();
}


TEST_F(gaia_se_references_test, remove_parent_reference__one_to_one) {
    begin_transaction();

    // GIVEN

    type_registry_t& registry = type_registry_t::instance();

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(registry);

    gaia_ptr parent = create_object(c_doctor_type, "Dr. House");
    gaia_ptr child = create_object(c_patient_type, "John Doe");

    parent.add_child_reference(child.id(), c_first_patient_offset);

    // WHEN

    child.remove_parent_reference(parent.id(), c_parent_doctor_offset);

    // THEN

    ASSERT_EQ(parent.references()[c_first_patient_offset], INVALID_GAIA_ID);
    ASSERT_EQ(child.references()[c_parent_doctor_offset], INVALID_GAIA_ID);
    ASSERT_EQ(child.references()[c_next_patient_offset], INVALID_GAIA_ID);

    commit_transaction();
}