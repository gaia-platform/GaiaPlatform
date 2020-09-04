/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include <sstream>

#include "gaia_common.hpp"

using gaia::common::gaia_exception;
using gaia::common::gaia_type_t;
using gaia::common::relation_offset_t;

namespace gaia::db {

/**
 * Denotes the cardinality of the children a parent can have.
 */
enum class cardinality_t {
    one,
    many
};

/**
 * Tells whether a child can be created without a parent.
 */
enum class modality_t {
    zero,
    one
};

/**
 * Describe one single relation between two types. By definition, a relation involves two types;
 * therefore, a reference to this structure will be associated with the types on both sides of
 * the relation.
 */
struct relation_t {
    gaia_type_t parent_type;
    gaia_type_t child_type;

    // Locates, in the prent payload, the pointer to the first child
    relation_offset_t first_child;

    // Locates, in the child payload, the pointer to the next child
    // or INVALID_GAIA_ID if this is the last children.
    relation_offset_t next_child;

    // Locates, in the child payload, the pointer to the parent
    relation_offset_t parent;

    cardinality_t cardinality;
    modality_t modality;
};

class invalid_relation_offset_t : public common::gaia_exception {
  public:
    invalid_relation_offset_t(gaia_type_t type, relation_offset_t offset) {
        stringstream message;
        message << "Gaia type \"" << type << "\" has no relations for the offset \"" << offset << "\"";
        m_message = message.str();
    }
};

class invalid_relation_type_t : public common::gaia_exception {
  public:
    invalid_relation_type_t(relation_offset_t offset, gaia_type_t expected_type, gaia_type_t found_type) {
        stringstream message;
        message << "Relation with offset \"" << offset << "\" requires type \"" << expected_type << "\" but found \"" << found_type << "\" ";
        m_message = message.str();
    }
};

class single_cardinality_violation_t : public common::gaia_exception {
  public:
    single_cardinality_violation_t(gaia_type_t type, relation_offset_t offset) {
        stringstream message;
        message << "Gaia type \"" << type << "\" has single cardinality for the relation with offset \"" << offset << "\"  but multiple children are being added";
        m_message = message.str();
    }
};

class child_already_in_relation_t : public common::gaia_exception {
  public:
    child_already_in_relation_t(gaia_type_t child_type, relation_offset_t offset) {
        stringstream message;
        message << "Gaia type \"" << child_type << "\" has already a reference for the relation with offset \"" << offset << "\"";
        m_message = message.str();
    }
};

} // namespace gaia::db
