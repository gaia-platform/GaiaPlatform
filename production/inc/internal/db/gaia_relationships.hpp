/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <sstream>

#include "gaia_common.hpp"

using gaia::common::gaia_exception;
using gaia::common::gaia_exception;
using gaia::common::reference_offset_t;

namespace gaia::db {

/**
 * Denotes the cardinality of the children a parent can have.
 */
enum class cardinality_t {
    not_set,
    one,
    many
};

/**
 * A relationship describes the logical connection between two Gaia containers:
 *  - A relationship always has a parent container and a child container.
 *  - A parent can be connected to one or more children (cardinality).
 *  - A parent can always be created before the children, a children may require
 *   the parent to exists in order to be created (modality).
 *  - A child can only have one parent.
 *
 * Instances of a relationship are referred to as references. Gaia objects
 * express references to other objects as pointers:
 *
 * (parent)-[first_child_offset]->(child)
 * (child)-[next_child_offset]->(child)
 * (child)-[parent_offset]->(parent)
 *
 * By definition, a relationship involves two containers; therefore, a reference to the
 * same relationship_t will be associated to both parent and child container.
 */
struct relationship_t {
    gaia_container_id_t parent_container;
    gaia_container_id_t child_container;

    // Locates, in the parent reference array, the pointer to the first child
    reference_offset_t first_child_offset;

    // Locates, in the child reference array, the pointer to the next child
    reference_offset_t next_child_offset;

    // Locates, in the child reference array, the pointer to the parent
    reference_offset_t parent_offset;

    cardinality_t cardinality;
    bool parent_required;
};

/**
 * Thrown when adding a reference to an offset that does not exist. It could surface the
 * user when a relationship is deleted at runtime, but the EDC classes are not up to
 * date with it.
 */
class invalid_reference_offset : public gaia_exception {
public:
    invalid_reference_offset(gaia_container_id_t container_id, reference_offset_t offset) {
        stringstream message;
        message << "Gaia container with id \"" << container_id << "\" has no relationship for the offset \"" << offset << "\"";
        m_message = message.str();
    }
};

/**
 * Thrown when adding a reference to an offset that exists but the object that is being
 * added belongs to the wrong container, according to the relationship definition.
 * This can happen when the relationships are modified at runtime and the EDC classes
 * are not up to date with it.
 */
class invalid_relationship_container : public gaia_exception {
public:
    invalid_relationship_container(reference_offset_t offset, gaia_container_id_t expected_container, gaia_container_id_t found_container) {
        stringstream message;
        message << "Relationship with offset \"" << offset << "\" requires container \"" << expected_container << "\" but found \"" << found_container << "\" ";
        m_message = message.str();
    }
};

/**
 * Thrown when adding more than one children to a one-to-one relationship.
 * This can happen when the relationships are modified at runtime and the EDC classes
 * are not up to date with it.
 */
class single_cardinality_violation : public gaia_exception {
public:
    single_cardinality_violation(gaia_container_id_t container_id, reference_offset_t offset) {
        stringstream message;
        message << "Gaia container_ with id \"" << container_id << "\" has single cardinality for the relationship with offset \"" << offset << "\"  but multiple children are being added";
        m_message = message.str();
    }
};

/**
 * Thrown when the adding a child to a relationship that already contains it.
 */
class child_already_referenced : public gaia_exception {
public:
    child_already_referenced(gaia_container_id_t child_container, reference_offset_t offset) {
        stringstream message;
        message << "Gaia container \"" << child_container << "\" has already a reference for the relationship with offset \"" << offset << "\"";
        m_message = message.str();
    }
};

} // namespace gaia::db
