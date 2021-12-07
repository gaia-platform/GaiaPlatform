/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia
{
namespace db
{

/**
 * Denotes the cardinality of the children a parent can have.
 */
enum class cardinality_t
{
    not_set,
    one,
    many
};

/**
 * A relationship describes the logical connection between two Gaia types:
 *  - A relationship always has a parent type and a child type.
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
 * By definition, a relationship involves two types; therefore, a reference to this
 * structure will be associated with the types on both sides of the relationship.
 */
struct relationship_t
{
    gaia::common::gaia_type_t parent_type;
    gaia::common::gaia_type_t child_type;

    // Locates, in the parent reference array, the pointer to the first child
    gaia::common::reference_offset_t first_child_offset;

    // Locates, in the child reference array, the pointer to the next child
    gaia::common::reference_offset_t next_child_offset;

    // Locates, in the child reference array, the pointer to the parent
    gaia::common::reference_offset_t parent_offset;

    cardinality_t cardinality;
    bool parent_required;
};

} // namespace db
} // namespace gaia
