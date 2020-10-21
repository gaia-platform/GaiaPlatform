/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "metadata_test_util.hpp"
#include "type_metadata.hpp"

using namespace std;
using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::db::test;

class gaia_relationships_test : public ::testing::Test
{
    void TearDown() override
    {
        clean_type_registry();
    }
};

TEST_F(gaia_relationships_test, metadata_one_to_many)
{
    type_registry_t& test_registry = type_registry_t::instance();

    test_relationship_builder_t::one_to_many()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto& parent = test_registry.get(c_doctor_type);
    auto& child = test_registry.get(c_patient_type);

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
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::many);
}

TEST_F(gaia_relationships_test, metadata_one_to_one)
{
    type_registry_t& test_registry = type_registry_t::instance();

    test_relationship_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto& parent = test_registry.get(c_doctor_type);
    auto& child = test_registry.get(c_patient_type);

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
    ASSERT_EQ(parent_rel->first_child_offset, c_first_patient_offset);
    ASSERT_EQ(parent_rel->next_child_offset, c_next_patient_offset);
    ASSERT_EQ(parent_rel->parent_offset, c_parent_doctor_offset);
    ASSERT_EQ(parent_rel->parent_required, false);
    ASSERT_EQ(parent_rel->cardinality, cardinality_t::one);
}

TEST_F(gaia_relationships_test, child_relation_do_not_use_next_child)
{
    type_registry_t& test_registry = type_registry_t::instance();

    test_relationship_builder_t::one_to_one()
        .parent(c_doctor_type)
        .child(c_patient_type)
        .create_relationship();

    auto& child = test_registry.get(c_patient_type);
    // although next_patient offset exists in child, it is not the one used
    // to identify the relation
    auto child_rel = child.find_child_relationship(c_next_patient_offset);

    ASSERT_TRUE(child_rel == nullptr);
}
