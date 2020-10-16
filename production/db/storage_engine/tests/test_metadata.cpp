/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "metadata_test_util.hpp"
#include "type_metadata.hpp"

using namespace gaia::db::test;

class gaia_metadata_test : public ::testing::Test
{
    void TearDown() override
    {
        clean_type_registry();
    }
};

TEST_F(gaia_metadata_test, get_throws_if_metadata_not_exist)
{
    ASSERT_THROW(
        type_registry_t::instance().get(c_non_existent_type),
        metadata_not_found);
}

TEST_F(gaia_metadata_test, creates_metadata_when_type_does_not_exist)
{
    auto metadata = type_registry_t::instance().get_or_create(c_non_existent_type);
    ASSERT_EQ(metadata.get_type(), c_non_existent_type);
}

TEST_F(gaia_metadata_test, add_thorws_if_metadata_already_exist)
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

TEST_F(gaia_metadata_test, edit_throws_if_metadata_not_exist)
{
    const auto relationship = make_shared<relationship_t>();

    ASSERT_THROW(
        type_registry_t::instance()
            .update(c_non_existent_type, [&](type_metadata_t& metadata) {
                metadata.add_parent_relationship(c_parent_doctor_offset, relationship);
            }),
        metadata_not_found);
}

TEST_F(gaia_metadata_test, remvoes_metada)
{
    type_registry_t::instance()
        .add(new type_metadata_t(c_patient_type));

    type_registry_t::instance().remove(c_patient_type);

    ASSERT_THROW(
        type_registry_t::instance().get(c_patient_type),
        metadata_not_found);
}

TEST_F(gaia_metadata_test, remvoes_metada_thorws_if_metadata_not_exist)
{
    ASSERT_THROW(
        type_registry_t::instance().remove(c_non_existent_type),
        metadata_not_found);
}
