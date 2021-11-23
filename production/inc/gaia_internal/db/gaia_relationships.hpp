/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

namespace gaia::db
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

/**
 * Thrown when adding a reference to an offset that does not exist. It could surface the
 * user when a relationship is deleted at runtime, but the DAC classes are not up to
 * date with it.
 */
class invalid_reference_offset : public gaia::common::gaia_exception
{
public:
    invalid_reference_offset(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
    {
        std::stringstream message;
        message << "Gaia type '" << type << "' has no relationship for the offset '" << offset << "'.";
        m_message = message.str();
    }
};

/**
 * Thrown when adding a reference to an offset that exists but the type of the object
 * that is being added is of the wrong type according to the relationship definition.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class invalid_relationship_type : public gaia::common::gaia_exception
{
public:
    invalid_relationship_type(
        gaia::common::reference_offset_t offset,
        gaia::common::gaia_type_t expected_type,
        gaia::common::gaia_type_t found_type)
    {
        std::stringstream message;
        message
            << "Relationship with offset '" << offset << "' requires type '" << expected_type
            << "' but found type '" << found_type << "'.";
        m_message = message.str();
    }
};

/**
 * Thrown when adding more than one children to a one-to-one relationship.
 * This can happen when the relationships are modified at runtime and the DAC classes
 * are not up to date with it.
 */
class single_cardinality_violation : public gaia::common::gaia_exception
{
public:
    single_cardinality_violation(gaia::common::gaia_type_t type, gaia::common::reference_offset_t offset)
    {
        std::stringstream message;
        message
            << "Gaia type '" << type << "' has single cardinality for the relationship with offset '" << offset
            << "', but multiple children are being added.";
        m_message = message.str();
    }
};

/**
 * Thrown when the adding a child to a relationship that already contains it.
 */
class child_already_referenced : public gaia::common::gaia_exception
{
public:
    child_already_referenced(gaia::common::gaia_type_t child_type, gaia::common::reference_offset_t offset)
    {
        std::stringstream message;
        message
            << "Gaia type '" << child_type
            << "' already has a reference for the relationship with offset '" << offset << "'.";
        m_message = message.str();
    }
};

class invalid_child : public gaia::common::gaia_exception
{
public:
    invalid_child(
        gaia::common::gaia_type_t child_type,
        gaia::common::gaia_id_t child_id,
        gaia::common::gaia_type_t parent_type,
        gaia::common::gaia_id_t parent_id)
    {
        std::stringstream message;
        message << "Impossible to remove child with id '" << child_id
                << "' and type '" << child_type
                << "' from parent with id '" << parent_id
                << "' and type '" << parent_type << "'. The child has a different parent.";
        m_message = message.str();
    }
};

} // namespace gaia::db
