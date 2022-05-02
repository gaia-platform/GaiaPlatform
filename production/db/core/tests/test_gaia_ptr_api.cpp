/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include <gaia/common.hpp>
#include <gaia/exceptions.hpp>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_ddl_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_test_util.hpp"

using namespace std;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::db::test;
using namespace gaia::common;

class gaia_ptr_api_test : public db_ddl_test_base_t
{
    void SetUp() override
    {
        db_ddl_test_base_t::SetUp();

        gaia_id_t doctor_table_id = create_table("doctor", {});
        gaia_id_t patient_table_id = create_table("patient", {});
        gaia_id_t clinic_table_id = create_table("clinic", {});
        gaia_id_t address_table_id = create_table("address", {});

        begin_transaction();
        doctor_type = gaia_table_t::get(doctor_table_id).type();
        patient_type = gaia_table_t::get(patient_table_id).type();
        clinic_type = gaia_table_t::get(clinic_table_id).type();
        address_type = gaia_table_t::get(address_table_id).type();
        commit_transaction();
    }

    void TearDown() override
    {
        db_test_base_t::TearDown();
        clean_type_registry();
    }

protected:
    static constexpr gaia_id_t c_non_existent_id = 10000000;

    gaia_type_t doctor_type;
    gaia_type_t patient_type;
    gaia_type_t clinic_type;
    gaia_type_t address_type;
};

TEST_F(gaia_ptr_api_test, creation_fail_for_invalid_type)
{
    begin_transaction();
    {
        const gaia_type_t c_invalid_type = 8888;
        EXPECT_THROW(gaia_ptr::create(c_invalid_type, 0, nullptr), invalid_object_type);
    }
    commit_transaction();
}

TEST_F(gaia_ptr_api_test, delete__one_to_many)
{
    begin_transaction();
    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
    commit_transaction();

    begin_transaction();
    ASSERT_THROW(gaia_ptr::remove(parent), object_still_referenced);
    commit_transaction();

    begin_transaction();
    ASSERT_NO_THROW(gaia_ptr::remove(child));
    ASSERT_NO_THROW(gaia_ptr::remove(parent));
    commit_transaction();
}

TEST_F(gaia_ptr_api_test, force_delete__one_to_many)
{
    begin_transaction();
    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
    commit_transaction();

    begin_transaction();
    ASSERT_NO_THROW(gaia_ptr::remove(parent, true));
    ASSERT_NO_THROW(gaia_ptr::remove(child));
    commit_transaction();
}

TEST_F(gaia_ptr_api_test, delete__one_to_one)
{
    begin_transaction();
    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
    commit_transaction();

    begin_transaction();
    ASSERT_NO_THROW(gaia_ptr::remove(parent));
    ASSERT_NO_THROW(gaia_ptr::remove(child));
    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__one_to_one)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);

    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], parent.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], anchor.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__one_to_many)
{
    begin_transaction();

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");
    gaia_ptr_t child2 = create_object(patient_type, "Jane Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
    gaia_ptr::insert_into_reference_container(parent, child2.id(), c_first_patient_offset);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);

    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child2.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], parent.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], anchor.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], child2.id());
    ASSERT_EQ(child2.references()[c_parent_doctor_offset], anchor.id());
    ASSERT_EQ(child2.references()[c_next_patient_offset], child.id());
    ASSERT_EQ(child2.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__single_cardinality_violation)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");
    gaia_ptr_t child2 = create_object(patient_type, "Jane Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(parent, child2.id(), c_first_patient_offset),
        single_cardinality_violation);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__invalid_relation_offset)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(parent, child.id(), c_non_existent_offset),
        invalid_reference_offset);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__invalid_relation_type_parent)
{
    begin_transaction();

    constexpr reference_offset_t c_first_address_offset = c_parent_doctor_offset.value() + 1;
    constexpr reference_offset_t c_next_address_offset = 0;
    constexpr reference_offset_t c_parent_patient_offset = 1;

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    // Need to create the relationship to create the pointers in the payload
    // otherwise the invalid_reference_offset would be thrown.
    relationship_builder_t::one_to_one()
        .parent(patient_type)
        .child(address_type)
        .first_child_offset(c_first_address_offset)
        .next_child_offset(c_next_address_offset)
        .parent_offset(c_parent_patient_offset)
        .create_relationship();

    gaia_ptr_t doctor = create_object(doctor_type, "Dr. House");
    gaia_ptr_t patient = create_object(patient_type, "Jane Doe");

    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(patient, doctor.id(), c_first_address_offset),
        invalid_relationship_type);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__invalid_relation_type_child)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t doctor = create_object(doctor_type, "Dr. House");
    gaia_ptr_t clinic = create_object(clinic_type, "Buena Vista Urgent Care");

    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(doctor, clinic.id(), c_first_patient_offset),
        invalid_relationship_type);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__child_already_in_relation)
{
    begin_transaction();

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    // doesn't fail if the same child is added to the same parent
    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t parent2 = create_object(doctor_type, "JD");
    gaia_ptr_t child2 = create_object(patient_type, "Jane Doe");

    gaia_ptr::insert_into_reference_container(parent2, child2.id(), c_first_patient_offset);

    // fails if a child that is already part of a relationship is added to a different parent
    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(parent, child2.id(), c_first_patient_offset),
        child_already_referenced);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, add_child_reference__invalid_object_id)
{
    begin_transaction();

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");

    EXPECT_THROW(
        gaia_ptr::insert_into_reference_container(parent, c_non_existent_id, c_first_patient_offset),
        invalid_object_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__one_to_one)
{
    begin_transaction();

    // GIVEN

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    // WHEN

    gaia_ptr::remove_from_reference_container(parent, child.id(), c_first_patient_offset);

    // THEN

    ASSERT_EQ(parent.references()[c_first_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_parent_doctor_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__many_to_many_from_back)
{
    begin_transaction();

    // GIVEN

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");

    vector<gaia_ptr_t> children;

    for (auto patient_name : {"Jhon Doe", "Jane Doe", "Foo", "Bar"})
    {
        gaia_ptr_t child = create_object(patient_type, patient_name);
        gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
        children.push_back(child);
    }

    // add_child_reference adds to the head
    for (size_t i = children.size() - 1; i < children.size(); --i)
    {
        auto child = children[i];

        // WHEN
        gaia_ptr::remove_from_reference_container(parent, child.id(), c_first_patient_offset);

        // THEN
        if (i > 0)
        {
            gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);
            ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], children[i - 1].id());
        }
        ASSERT_EQ(child.references()[c_parent_doctor_offset], c_invalid_gaia_id);
        ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
        ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);
    }

    // when the last child is removed the parent needs to be updated as well.
    ASSERT_EQ(parent.references()[c_first_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__many_to_many_from_head)
{
    begin_transaction();

    // GIVEN

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");

    vector<gaia_ptr_t> children;

    for (auto patient_name : {"Jhon Doe", "Jane Doe", "Foo", "Bar"})
    {
        gaia_ptr_t child = create_object(patient_type, patient_name);
        gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);
        children.push_back(child);
    }

    // add_child_reference adds to the head
    for (const auto& child : children)
    {
        // WHEN
        gaia_ptr::remove_from_reference_container(parent, child.id(), c_first_patient_offset);

        // THEN
        if (child.id() != children.back().id())
        {
            gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);
            ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], children.back().id());
        }
        ASSERT_EQ(child.references()[c_parent_doctor_offset], c_invalid_gaia_id);
        ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    }

    // when the last child is removed the parent needs to be updated as well.
    ASSERT_EQ(parent.references()[c_first_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__different_child)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");
    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t child2 = create_object(patient_type, "Jane Doe");

    ASSERT_FALSE(gaia_ptr::remove_from_reference_container(parent, child2.id(), c_first_patient_offset));

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);
    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], parent.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], anchor.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__different_parent)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");
    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t parent2 = create_object(doctor_type, "Jane Doe");

    // nothing should happen
    ASSERT_THROW(
        gaia_ptr::remove_from_reference_container(parent2, child.id(), c_first_patient_offset),
        invalid_child_reference);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);
    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], parent.id());
    ASSERT_EQ(child.references()[c_parent_doctor_offset], anchor.id());
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__invalid_relation_offset)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");
    gaia_ptr::remove_from_reference_container(parent, child.id(), c_first_patient_offset);

    EXPECT_THROW(
        gaia_ptr::remove_from_reference_container(parent, child.id(), c_non_existent_offset),
        invalid_reference_offset);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__invalid_object_id)
{
    begin_transaction();

    relationship_builder_t::one_to_many()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");

    EXPECT_THROW(
        gaia_ptr::remove_from_reference_container(parent, c_non_existent_id, c_first_patient_offset),
        invalid_object_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__invalid_relation_type_parent)
{
    begin_transaction();

    constexpr reference_offset_t c_first_address_offset = c_parent_doctor_offset.value() + 1;
    constexpr reference_offset_t c_next_address_offset = 0;
    constexpr reference_offset_t c_parent_patient_offset = 1;

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    // Need to create the relationship to create the pointers in the payload
    // otherwise the invalid_reference_offset would be thrown.
    relationship_builder_t::one_to_one()
        .parent(patient_type)
        .child(address_type)
        .first_child_offset(c_first_address_offset)
        .next_child_offset(c_next_address_offset)
        .parent_offset(c_parent_patient_offset)
        .create_relationship();

    gaia_ptr_t doctor = create_object(doctor_type, "Dr. House");
    gaia_ptr_t patient = create_object(patient_type, "Jane Doe");

    EXPECT_THROW(
        gaia_ptr::remove_from_reference_container(patient, doctor.id(), c_first_address_offset),
        invalid_relationship_type);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_child_reference__invalid_relation_type_child)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t doctor = create_object(doctor_type, "Dr. House");
    gaia_ptr_t clinic = create_object(clinic_type, "Buena Vista Urgent Care");

    EXPECT_THROW(
        gaia_ptr::remove_from_reference_container(doctor, clinic.id(), c_first_patient_offset),
        invalid_relationship_type);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, remove_parent_reference__one_to_one)
{
    begin_transaction();

    // GIVEN

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    // WHEN

    gaia_ptr::remove_from_reference_container(child, c_parent_doctor_offset);

    // THEN

    ASSERT_EQ(parent.references()[c_first_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_parent_doctor_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, update_parent_reference__parent_exists)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t new_parent = create_object(doctor_type, "Dr. Cox");
    gaia_ptr::update_parent_reference(child, new_parent.id(), c_parent_doctor_offset);

    ASSERT_EQ(parent.references()[c_first_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(new_parent.references()[c_first_patient_offset], child.references()[c_parent_doctor_offset]);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(new_parent.references()[c_first_patient_offset]);
    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], new_parent.id());

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, update_parent_reference__parent_not_exists)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr_t new_parent = create_object(doctor_type, "JD");
    gaia_ptr::update_parent_reference(child, new_parent.id(), c_parent_doctor_offset);

    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(new_parent.references()[c_first_patient_offset], child.references()[c_parent_doctor_offset]);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(new_parent.references()[c_first_patient_offset]);
    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], new_parent.id());

    commit_transaction();
}

TEST_F(gaia_ptr_api_test, update_parent_reference__single_cardinality_violation)
{
    begin_transaction();

    relationship_builder_t::one_to_one()
        .parent(doctor_type)
        .child(patient_type)
        .create_relationship();

    gaia_ptr_t parent = create_object(doctor_type, "Dr. House");
    gaia_ptr_t child = create_object(patient_type, "John Doe");

    gaia_ptr::insert_into_reference_container(parent, child.id(), c_first_patient_offset);

    gaia_ptr_t new_parent = create_object(doctor_type, "Dr. Cox");
    gaia_ptr_t new_child = create_object(patient_type, "Jane Doe");

    gaia_ptr::insert_into_reference_container(new_parent, new_child.id(), c_first_patient_offset);

    EXPECT_THROW(
        gaia_ptr::update_parent_reference(child, new_parent.id(), c_parent_doctor_offset),
        single_cardinality_violation);

    ASSERT_EQ(child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(child.references()[c_prev_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(new_child.references()[c_next_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(new_child.references()[c_prev_patient_offset], c_invalid_gaia_id);
    ASSERT_EQ(parent.references()[c_first_patient_offset], child.references()[c_parent_doctor_offset]);
    ASSERT_EQ(new_parent.references()[c_first_patient_offset], new_child.references()[c_parent_doctor_offset]);

    gaia_ptr_t anchor = gaia_ptr_t::from_gaia_id(parent.references()[c_first_patient_offset]);
    ASSERT_EQ(anchor.references()[c_ref_anchor_first_child_offset], child.id());
    ASSERT_EQ(anchor.references()[c_ref_anchor_parent_offset], parent.id());

    gaia_ptr_t new_anchor = gaia_ptr_t::from_gaia_id(new_parent.references()[c_first_patient_offset]);
    ASSERT_EQ(new_anchor.references()[c_ref_anchor_first_child_offset], new_child.id());
    ASSERT_EQ(new_anchor.references()[c_ref_anchor_parent_offset], new_parent.id());
    commit_transaction();
}
