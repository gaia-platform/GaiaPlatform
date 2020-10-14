/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "relations_test_util.hpp"
#include "type_metadata.hpp"

using namespace gaia::db::test;

class gaia_metadata_test : public ::testing::Test
{
    void TearDown() override
    {
        clean_type_registry();
    }
};

TEST_F(gaia_metadata_test, throws_error_on_get_metadata_not_exist)
{
    ASSERT_THROW(
        type_registry_t::instance().get(c_non_existent_type),
        metadata_not_found);
}

TEST_F(gaia_metadata_test, throws_error_on_add_metadata_already_exist)
{
    type_registry_t::instance().add(new type_metadata_t(c_patient_type));

    ASSERT_THROW(
        type_registry_t::instance().add(new type_metadata_t(c_patient_type)),
        duplicate_metadata);
}

TEST_F(gaia_metadata_test, edit_metadata)
{
    type_registry_t::instance()
        .add(new type_metadata_t(c_patient_type));

    const auto relationship = make_shared<relationship_t>();

    type_registry_t::instance()
        .update(c_patient_type, [&](type_metadata_t& metadata) {
            metadata.add_parent_relationship(c_parent_doctor_offset, relationship);
        });

    auto& metadata = type_registry_t::instance().get(c_patient_type);

    ASSERT_EQ(metadata.find_parent_relationship(c_parent_doctor_offset), relationship.get());
}

TEST_F(gaia_metadata_test, throws_error_on_edit_metadata_not_exist)
{
    const auto relationship = make_shared<relationship_t>();

    ASSERT_THROW(
        type_registry_t::instance()
            .update(c_non_existent_type, [&](type_metadata_t& metadata) {
                metadata.add_parent_relationship(c_parent_doctor_offset, relationship);
            }),
        metadata_not_found);
}
