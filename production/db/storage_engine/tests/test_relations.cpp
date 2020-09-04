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

TEST(relations, registry_creates_metadata_when_type_does_not_exist) {
    type_registry_t test_registry;
    auto metadata = test_registry.get_metadata(NON_EXISTENT_TYPE);
    ASSERT_EQ(metadata.get_type(), NON_EXISTENT_TYPE);
}


TEST(relations, metadata_one_to_many) {
    type_registry_t test_registry;

    relation_builder_t::one_to_many()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(test_registry);

    auto parent = test_registry.get_metadata(DOCTOR_TYPE);
    auto child = test_registry.get_metadata(PATIENT_TYPE);

    ASSERT_EQ(parent.get_type(), DOCTOR_TYPE);
    ASSERT_EQ(child.get_type(), PATIENT_TYPE);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel_opt = parent.find_parent_relation(FIRST_PATIENT_OFFSET);
    ASSERT_TRUE(parent_rel_opt.has_value());
    auto parent_rel = parent_rel_opt.value();

    auto child_rel_opt = child.find_child_relation(NEXT_PATIENT_OFFSET);
    ASSERT_TRUE(child_rel_opt.has_value());
    auto child_rel = child_rel_opt.value();

    // parent and child should be sharing the same relation
    // TODO these 2 should be the same but are somewhat different.
    // ASSERT_EQ(&parent_rel, &child_rel);

    ASSERT_EQ(parent_rel.parent_type, DOCTOR_TYPE);
    ASSERT_EQ(parent_rel.child_type, PATIENT_TYPE);
    ASSERT_EQ(parent_rel.first_child, FIRST_PATIENT_OFFSET);
    ASSERT_EQ(parent_rel.next_child, NEXT_PATIENT_OFFSET);
    ASSERT_EQ(parent_rel.parent, PARENT_DOCTOR_OFFSET);
    ASSERT_EQ(parent_rel.modality, modality_t::zero);
    ASSERT_EQ(parent_rel.cardinality, cardinality_t::many);
}

TEST(relations, metadata_one_to_one) {
    type_registry_t test_registry;

    relation_builder_t::one_to_one()
        .parent(DOCTOR_TYPE)
        .child(PATIENT_TYPE)
        .add_to_registry(test_registry);

    auto parent = test_registry.get_metadata(DOCTOR_TYPE);
    auto child = test_registry.get_metadata(PATIENT_TYPE);

    ASSERT_EQ(parent.get_type(), DOCTOR_TYPE);
    ASSERT_EQ(child.get_type(), PATIENT_TYPE);

    ASSERT_EQ(parent.num_references(), 1);
    ASSERT_EQ(child.num_references(), 2);

    auto parent_rel_opt = parent.find_parent_relation(FIRST_PATIENT_OFFSET);
    ASSERT_TRUE(parent_rel_opt.has_value());
    auto parent_rel = parent_rel_opt.value();

    auto child_rel_opt = child.find_child_relation(NEXT_PATIENT_OFFSET);
    ASSERT_TRUE(child_rel_opt.has_value());
    auto child_rel = child_rel_opt.value();

    // parent and child should be sharing the same relation
    // TODO these 2 should be the same but are somewhat different.
    // ASSERT_EQ(&parent_rel, &child_rel);

    ASSERT_EQ(parent_rel.parent_type, DOCTOR_TYPE);
    ASSERT_EQ(parent_rel.child_type, PATIENT_TYPE);
    ASSERT_EQ(parent_rel.first_child, FIRST_PATIENT_OFFSET);
    ASSERT_EQ(parent_rel.next_child, NEXT_PATIENT_OFFSET);
    ASSERT_EQ(parent_rel.parent, PARENT_DOCTOR_OFFSET);
    ASSERT_EQ(parent_rel.modality, modality_t::zero);
    ASSERT_EQ(parent_rel.cardinality, cardinality_t::one);
}