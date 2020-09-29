/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "container_metadata.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia::db::test {

// Here we model an hypothetical relationship between Doctors and Patients
// The relationship can be both 1:1 or 1:n depending on the test.
// Doctor --> Patient
// This relationship is used in most relationship/references tests.

constexpr gaia_container_id_t c_doctor_container = 1;
constexpr gaia_container_id_t c_patient_container = 2;
constexpr gaia_container_id_t c_non_existent_container = 1001;

constexpr gaia_id_t c_non_existent_id = 10000000;

constexpr reference_offset_t c_first_patient_offset = 0;
constexpr reference_offset_t c_next_patient_offset = 0;
constexpr reference_offset_t c_parent_doctor_offset = 1;
constexpr reference_offset_t c_non_existent_offset = 1024;

/**
 * Facilitate the creation of relationship objects and their insertion into
 * the registry.
 */
class relationship_builder_t {
public:
    static relationship_builder_t one_to_one() {
        auto metadata = relationship_builder_t();
        metadata.m_cardinality = cardinality_t::one;
        return metadata;
    }

    static relationship_builder_t one_to_many() {
        auto metadata = relationship_builder_t();
        metadata.m_cardinality = cardinality_t::many;
        return metadata;
    }

    relationship_builder_t parent(gaia_container_id_t parent) {
        this->m_parent_container = parent;
        return *this;
    }

    relationship_builder_t child(gaia_container_id_t child) {
        this->m_child_container = child;
        return *this;
    }

    relationship_builder_t first_child_offset(reference_offset_t first_child_offset) {
        this->m_first_child_offset = first_child_offset;
        return *this;
    }

    relationship_builder_t next_child_offset(reference_offset_t next_child_offset) {
        this->m_next_child_offset = next_child_offset;
        return *this;
    }

    relationship_builder_t parent_offset(reference_offset_t parent_offset) {
        this->m_parent_offset = parent_offset;
        return *this;
    }

    // Creates all the object necessary to describe the relationship and
    // updates the container registry.
    void create_relationship() {
        if (m_parent_container == INVALID_GAIA_CONTAINER) {
            throw invalid_argument("parent_container must be set");
        }

        if (m_child_container == INVALID_GAIA_CONTAINER) {
            throw invalid_argument("child_container must be set");
        }

        if (m_cardinality == cardinality_t::not_set) {
            throw invalid_argument("cardinality must be set");
        }


        auto rel = make_shared<relationship_t>(relationship_t{
            .parent_container = this->m_parent_container,
            .child_container = this->m_child_container,
            .first_child_offset = this->m_first_child_offset,
            .next_child_offset = this->m_next_child_offset,
            .parent_offset = this->m_parent_offset,
            .cardinality = this->m_cardinality,
            .parent_required = this->m_parent_required});

        auto& parent_meta = container_registry_t::instance().get_or_create(m_parent_container);
        parent_meta.add_parent_relationship(m_first_child_offset, rel);

        auto& child_meta = container_registry_t::instance().get_or_create(m_child_container);
        child_meta.add_child_relationship(m_parent_offset, rel);
    }

private:
    relationship_builder_t() = default;

    // mandatory values
    cardinality_t m_cardinality = cardinality_t::not_set;
    gaia_container_id_t m_parent_container = INVALID_GAIA_CONTAINER;
    gaia_container_id_t m_child_container = INVALID_GAIA_CONTAINER;

    // default values add methods for those.
    bool m_parent_required = false;
    reference_offset_t m_first_child_offset = c_first_patient_offset;
    reference_offset_t m_next_child_offset = c_next_patient_offset;
    reference_offset_t m_parent_offset = c_parent_doctor_offset;
};

/**
 * Creates a gaia_se_object_t using the container metadata to infer some of the details about the object
 * such as the number of relations.
 *
 * TODO maybe some of this logic should be moved inside gaia_ptr
 */
gaia_ptr create_object(gaia_container_id_t container_id, size_t data_size, const void* data) {
    gaia_id_t id = gaia_ptr::generate_id();
    auto metadata = container_registry_t::instance().get_or_create(container_id);
    size_t num_references = metadata.num_references();
    return gaia_ptr::create(id, container_id, num_references, data_size, data);
}

gaia_ptr create_object(gaia_container_id_t container_id, std::string payload) {
    return create_object(container_id, payload.size(), payload.data());
}

void clean_container_registry() {
    container_registry_t::instance().clear();
}

} // namespace gaia::db::test
