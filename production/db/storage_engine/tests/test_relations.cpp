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

TEST(relations, registry_creates_metadata_when_type_does_not_exist) {
    type_registry_t test_registry;
    auto metadata = test_registry.get_or_create(c_non_existent_type);
    ASSERT_EQ(metadata.get_type(), c_non_existent_type);
}

TEST(relations, metadata_one_to_many) {
    type_registry_t test_registry;

    relation_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(test_registry);

    auto& parent = test_registry.get_or_create(c_doctor_type);
    auto& child = test_registry.get_or_create(c_patient_type);

    ASSERT_EQ(parent.get_type(), c_doctor_type);
    ASSERT_EQ(child.get_type(), c_patient_type);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel != nullptr);

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel != nullptr);

    // Parent and child should be sharing the same relation.
    ASSERT_EQ(parent_rel, child_rel);

    ASSERT_EQ(parent_rel->parent_type, c_doctor_type);
    ASSERT_EQ(parent_rel->child_type, c_patient_type);
    ASSERT_EQ(parent_rel->first_child, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->modality, modality_t::zero);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::many);
}

TEST(relations, metadata_one_to_one) {
    type_registry_t test_registry;

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(test_registry);

    auto parent = test_registry.get_or_create(c_doctor_type);
    auto child = test_registry.get_or_create(c_patient_type);

    ASSERT_EQ(parent.get_type(), c_doctor_type);
    ASSERT_EQ(child.get_type(), c_patient_type);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel = parent.find_parent_relationship(c_first_patient_offset);
    ASSERT_TRUE(parent_rel != nullptr);

    auto child_rel = child.find_child_relationship(c_parent_doctor_offset);
    ASSERT_TRUE(child_rel != nullptr);

    // Parent and child should be sharing the same relation.
    ASSERT_EQ(parent_rel, child_rel);

    ASSERT_EQ(parent_rel->parent_type, c_doctor_type);
    ASSERT_EQ(parent_rel->child_type, c_patient_type);
    ASSERT_EQ(parent_rel->first_child, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->modality, modality_t::zero);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::one);
}

TEST(relations, child_relation_do_not_use_next_child) {
    type_registry_t test_registry;

    relation_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .add_to_registry(test_registry);

    auto child = test_registry.get_or_create(c_patient_type);
    // although next_patient offset exists in child, it is not the one used
    // to identify the relation
    auto child_rel = child.find_child_relationship(c_next_patient_offset);

    ASSERT_TRUE(child_rel == nullptr);
}
