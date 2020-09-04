//
// Created by simone on 9/4/20.
//

#pragma once

#include "type_metadata.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::db;
using namespace gaia::common;

// Here we model an hypothetical relation between Doctors and Patients
// The relation can be both 1:1 or 1:n depending on the test.
// Doctor --> Patient
// This relation is used in most relation/references tests.

constexpr gaia_type_t DOCTOR_TYPE = 0;
constexpr gaia_type_t PATIENT_TYPE = 1;
constexpr gaia_type_t NON_EXISTENT_TYPE = 1001;

constexpr relation_offset_t FIRST_PATIENT_OFFSET = 0;
constexpr relation_offset_t NEXT_PATIENT_OFFSET = 0;
constexpr relation_offset_t PARENT_DOCTOR_OFFSET = 1;
constexpr relation_offset_t NON_EXISTENT_OFFSET = 1024;

/**
 * Facilitate the creation of relation objects and their insertion into
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

    relation_builder_t first_child_offset(relation_offset_t first_child) {
        this->m_first_child = first_child;
        return *this;
    }

    relation_builder_t next_child_offset(relation_offset_t next_child) {
        this->m_next_child = next_child;
        return *this;
    }

    relation_builder_t parent_offset(relation_offset_t parent) {
        this->m_parent = parent;
        return *this;
    }

    // does not use the singleton instance to avoid strange test
    // behaviors
    void add_to_registry(type_registry_t &registry) {
        if (m_parent_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("parent_type must be set");
        }

        if (m_child_type == INVALID_GAIA_TYPE) {
            throw invalid_argument("child_type must be set");
        }

        auto rel = make_shared<relation_t>(relation_t{
            .parent_type = this->m_parent_type,
            .child_type = this->m_child_type,
            .first_child = this->m_first_child,
            .next_child = this->m_next_child,
            .parent = this->m_parent,
            .cardinality = this->m_cardinality,
            .modality = this->m_modality});

        // TODO don't understand why if I use auto here, it returns a copy and not a reference.
        type_metadata_t &parent_meta = registry.get_metadata(m_parent_type);
        parent_meta.add_parent_relation(m_first_child, rel);

        type_metadata_t &child_meta = registry.get_metadata(m_child_type);
        child_meta.add_child_relation(m_next_child, rel);
    }

  private:
    relation_builder_t() = default;

    // mandatory values
    cardinality_t m_cardinality = cardinality_t::one;
    gaia_type_t m_parent_type = INVALID_GAIA_TYPE;
    gaia_type_t m_child_type = INVALID_GAIA_TYPE;

    // default values add methods for those.
    modality_t m_modality = modality_t::zero;
    relation_offset_t m_first_child = FIRST_PATIENT_OFFSET;
    relation_offset_t m_next_child = NEXT_PATIENT_OFFSET;
    relation_offset_t m_parent = PARENT_DOCTOR_OFFSET;
};

/**
 * Creates a gaia_se_object_t using the type metadata to infer some of the details about the object
 * such as the number of relations.
 *
 * TODO maybe some of this logic should be moved inside gaia_ptr
 */
gaia_ptr create_object(gaia_type_t type, size_t data_size, const void *data) {
    gaia_id_t id = gaia_ptr::generate_id();
    auto metadata = type_registry_t::instance().get_metadata(type);
    size_t num_references = metadata.num_references();
    return gaia_ptr::create(id, type, num_references, data_size, data);
}

gaia_ptr create_object(gaia_type_t type, std::string payload) {
    return create_object(type, payload.size(), payload.data());
}
