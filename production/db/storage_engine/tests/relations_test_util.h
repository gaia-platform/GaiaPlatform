/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////


#pragma once

#include "type_metadata.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::db;
using namespace gaia::common;

namespace gaia::db::test {

// Here we model an hypothetical relationship between Doctors and Patients
// The relationship can be both 1:1 or 1:n depending on the test.
// Doctor --> Patient
// This relationship is used in most relationship/references tests.

constexpr gaia_type_t c_doctor_type = 1;
constexpr gaia_type_t c_patient_type = 2;
constexpr gaia_type_t c_non_existent_type = 1001;

constexpr gaia_id_t c_non_existent_id = 10000000;

constexpr relationship_offset_t c_first_patient_offset = 0;
constexpr relationship_offset_t c_next_patient_offset = 0;
constexpr relationship_offset_t c_parent_doctor_offset = 1;
constexpr relationship_offset_t c_non_existent_offset = 1024;

/**
 * Facilitate the creation of relationship objects and their insertion into
 * the registry.
 */
class relation_builder_t {
  public:
    static relation_builder_t one_to_one() {
        auto metadata = relation_builder_t();
        metadata.m_cardinality = cardinality_t::one;
        return metadata;
    }

    static relation_builder_t one_to_many() {
        auto metadata = relation_builder_t();
        metadata.m_cardinality = cardinality_t::many;
        return metadata;
    }

    relation_builder_t parent(gaia_type_t parent) {
        this->m_parent_type = parent;
        return *this;
    }

    relation_builder_t child(gaia_type_t child) {
        this->m_child_type = child;
        return *this;
    }

    relation_builder_t first_child_offset(relationship_offset_t first_child) {
        this->m_first_child = first_child;
        return *this;
    }

    relation_builder_t next_child_offset(relationship_offset_t next_child) {
        this->m_next_child = next_child;
        return *this;
    }

    relation_builder_t parent_offset(relationship_offset_t parent) {
        this->m_parent = parent;
        return *this;
    }

    // does not use the singleton instance to avoid strange test
    // behaviors
    void add_to_registry(type_registry_t& registry) {
        if (m_parent_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("parent_type must be set");
        }

        if (m_child_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("child_type must be set");
        }

        if (m_cardinality == cardinality_t::not_set) {
            throw invalid_argument("cardinality must be set");
        }

        auto rel = make_shared<relationship_t>(relationship_t{
            .parent_type = this->m_parent_type,
            .child_type = this->m_child_type,
            .first_child = this->m_first_child,
            .next_child = this->m_next_child,
            .parent = this->m_parent,
            .cardinality = this->m_cardinality,
            .modality = this->m_modality});

        auto& parent_meta = registry.get_or_create(m_parent_type);
        parent_meta.add_parent_relationship(m_first_child, rel);

        auto& child_meta = registry.get_or_create(m_child_type);
        child_meta.add_child_relationship(m_parent, rel);
    }

  private:
    relation_builder_t() = default;

    // mandatory values
    cardinality_t m_cardinality = cardinality_t::not_set;
    gaia_type_t m_parent_type = INVALID_GAIA_TYPE;
    gaia_type_t m_child_type = INVALID_GAIA_TYPE;

    // default values add methods for those.
    modality_t m_modality = modality_t::zero;
    relationship_offset_t m_first_child = c_first_patient_offset;
    relationship_offset_t m_next_child = c_next_patient_offset;
    relationship_offset_t m_parent = c_parent_doctor_offset;
};

/**
 * Creates a gaia_se_object_t using the type metadata to infer some of the details about the object
 * such as the number of relations.
 *
 * TODO maybe some of this logic should be moved inside gaia_ptr
 */
gaia_ptr create_object(gaia_type_t type, size_t data_size, const void* data) {
    gaia_id_t id = gaia_ptr::generate_id();
    auto metadata = type_registry_t::instance().get_or_create(type);
    size_t num_references = metadata.num_references();
    return gaia_ptr::create(id, type, num_references, data_size, data);
}

gaia_ptr create_object(gaia_type_t type, std::string payload) {
    return create_object(type, payload.size(), payload.data());
}

void clean_type_registry() {
    type_registry_t::instance().clear();
}

} // namespace gaia::db::test
