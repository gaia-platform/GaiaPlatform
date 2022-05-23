////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/type_metadata.hpp"

namespace gaia::db::test
{

// Here we model an hypothetical relationship between Doctors and Patients
// The relationship can be both 1:1 or 1:n depending on the test.
// Doctor --> Patient
// This relationship is used in most relationship/references tests.
constexpr common::reference_offset_t c_first_patient_offset = 0;
constexpr common::reference_offset_t c_parent_doctor_offset = 0;
constexpr common::reference_offset_t c_next_patient_offset = 1;
constexpr common::reference_offset_t c_prev_patient_offset = 2;
constexpr common::reference_offset_t c_non_existent_offset = 1024;

/**
 * Facilitate the creation of relationship objects and their insertion into
 * the registry. It also creates the instances of type_metadata_t in
 * the registry if not available.
 */
class relationship_builder_t
{
public:
    static relationship_builder_t one_to_one()
    {
        auto metadata = relationship_builder_t();
        metadata.m_cardinality = cardinality_t::one;
        return metadata;
    }

    static relationship_builder_t one_to_many()
    {
        auto metadata = relationship_builder_t();
        metadata.m_cardinality = cardinality_t::many;
        return metadata;
    }

    relationship_builder_t parent(common::gaia_type_t parent)
    {
        this->m_parent_type = parent;
        return *this;
    }

    relationship_builder_t child(common::gaia_type_t child)
    {
        this->m_child_type = child;
        return *this;
    }

    relationship_builder_t first_child_offset(common::reference_offset_t first_child_offset)
    {
        this->m_first_child_offset = first_child_offset;
        return *this;
    }

    relationship_builder_t next_child_offset(common::reference_offset_t next_child_offset)
    {
        this->m_next_child_offset = next_child_offset;
        return *this;
    }

    relationship_builder_t prev_child_offset(common::reference_offset_t prev_child_offset)
    {
        this->m_prev_child_offset = prev_child_offset;
        return *this;
    }

    relationship_builder_t parent_offset(common::reference_offset_t parent_offset)
    {
        this->m_parent_offset = parent_offset;
        return *this;
    }

    // Creates all the object necessary to describe the relationship and
    // updates the type registry.
    void create_relationship()
    {
        if (!m_parent_type.is_valid())
        {
            throw std::invalid_argument("parent_type must be set!");
        }

        if (!m_child_type.is_valid())
        {
            throw std::invalid_argument("child_type must be set!");
        }

        if (m_cardinality == cardinality_t::not_set)
        {
            throw std::invalid_argument("cardinality must be set!");
        }

        auto rel = std::make_shared<relationship_t>(relationship_t{
            .parent_type = this->m_parent_type,
            .child_type = this->m_child_type,
            .first_child_offset = this->m_first_child_offset,
            .next_child_offset = this->m_next_child_offset,
            .prev_child_offset = this->m_prev_child_offset,
            .parent_offset = this->m_parent_offset,
            .cardinality = this->m_cardinality,
            .parent_required = this->m_parent_required});

        auto& parent_meta = type_registry_t::instance().test_get_or_create(m_parent_type);
        parent_meta.add_parent_relationship(rel);

        auto& child_meta = type_registry_t::instance().test_get_or_create(m_child_type);
        child_meta.add_child_relationship(rel);
    }

private:
    relationship_builder_t() = default;

    // mandatory values
    cardinality_t m_cardinality = cardinality_t::not_set;
    common::gaia_type_t m_parent_type = common::c_invalid_gaia_type;
    common::gaia_type_t m_child_type = common::c_invalid_gaia_type;

    // default values add methods for those.
    bool m_parent_required = false;
    common::reference_offset_t m_first_child_offset = c_first_patient_offset;
    common::reference_offset_t m_next_child_offset = c_next_patient_offset;
    common::reference_offset_t m_prev_child_offset = c_prev_patient_offset;
    common::reference_offset_t m_parent_offset = c_parent_doctor_offset;
};

gaia_ptr_t create_object(common::gaia_type_t type, std::string payload)
{
    return gaia_ptr::create(type, payload.size(), payload.data());
}

void clean_type_registry()
{
    type_registry_t::instance().clear();
}

} // namespace gaia::db::test
