/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"
#include "gtest/gtest.h"
#include "relations_test_util.h"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::db::test;

class gaia_relationships_test : public ::testing::Test {
    void TearDown() override {
        clean_type_registry();
    }
};

TEST_F(gaia_relationships_test, registry_creates_metadata_when_type_does_not_exist) {
    auto metadata = container_registry_t::instance().get_or_create(c_non_existent_type);
    ASSERT_EQ(metadata.get_container_id(), c_non_existent_type);
}

TEST_F(gaia_relationships_test, metadata_one_to_many) {
    container_registry_t& test_registry = container_registry_t::instance();

    relationship_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto& parent = test_registry.get_or_create(c_doctor_type);
    auto& child = test_registry.get_or_create(c_patient_type);

    ASSERT_EQ(parent.get_container_id(), c_doctor_type);
    ASSERT_EQ(child.get_container_id(), c_patient_type);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel != nullptr);

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel != nullptr);

    // Parent and child should be sharing the same relation.
    ASSERT_EQ(parent_rel, child_rel);

    ASSERT_EQ(parent_rel->parent_container, c_doctor_type);
    ASSERT_EQ(parent_rel->child_container, c_patient_type);
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::many);
}

TEST_F(gaia_relationships_test, metadata_one_to_one) {
    container_registry_t& test_registry = container_registry_t::instance();

    relationship_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto parent = test_registry.get_or_create(c_doctor_type);
    auto child = test_registry.get_or_create(c_patient_type);

    ASSERT_EQ(parent.get_container_id(), c_doctor_type);
    ASSERT_EQ(child.get_container_id(), c_patient_type);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel != nullptr);

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel != nullptr);

    // Parent and child should be sharing the same relation.
    ASSERT_EQ(parent_rel, child_rel);

    ASSERT_EQ(parent_rel->parent_container, c_doctor_type);
    ASSERT_EQ(parent_rel->child_container, c_patient_type);
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::one);
}

TEST_F(gaia_relationships_test, child_relation_do_not_use_next_child) {
    container_registry_t& test_registry = container_registry_t::instance();

    relationship_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto child = test_registry.get_or_create(c_patient_type);
    // although next_patient offset exists in child, it is not the one used
    // to identify the relation
    auto child_rel = child.find_child_relationship(c_next_patient_offset);

    ASSERT_TRUE(child_rel == nullptr);
}
