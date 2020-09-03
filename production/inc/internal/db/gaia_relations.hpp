/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once
#include "gaia_common.hpp"

using gaia::common::gaia_type_t;
using gaia::common::relation_offset_t;

namespace gaia {
namespace db {

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
 * therefore, a reference to this structure will be available on both sides of the relation.
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

} // namespace db
} // namespace gaia
