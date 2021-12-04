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
 * TODO: Create sub-classes for different types of relationships.
 *
 * A relationship describes the logical connection between two Gaia types, whose
 * direct access classes have methods to allow objects of the given type to
 * visit the other object in the relationship directly.

 * To facilitate the efficient traversal of objects in relationships, database
 * will allocate reference slots for each relationship the object is in.
 *
 * The objects in a relationship can form two different typologies from the
 * references that connect them: one with an anchor node and one without.
 *
 * A reference chain without a anchor node will look like the following graph.
 *
 *          +----------+
 *          v          |
 *         +----+     +----+     +----+     +----+
 *      +> | P  | --> | C1 | --> | C2 | --> | C3 |
 *      |  +----+     +----+     +----+     +----+
 *      |    ^                     |          |
 *      |    +---------------------+          |
 *      |                                     |
 *      |                                     |
 *      +-------------------------------------+
 *
 * In the above graph, P is a parent node; C1, C2, and C3 are child nodes. A
 * parent node has one reference at the 'first_child_offset' slot pointing to
 * the first child node in the list. Each child node has two references: one at
 * the 'next_child_offset' slot pointing to the next child in the list; another
 * at the 'parent_offset' slot pointing to the parent node.
 *
 * A reference chain with anchor node will look like the following graph.
 *
 *              +------------------------------------+
 *              |                                    |
 *              |                                    |
 *              |    +--------------------+          |
 *              v    v                    |          |
 *  +----+     +--------+     +----+     +----+     +----+
 *  | P  | --> |        | --> |    | --> |    | --> |    |
 *  +----+     |        |     |    |     |    |     |    |
 *    ^        |        |     |    |     |    |     |    |
 *    +------- |   A    | <-- | C1 | <-- | C2 | <-- | C3 |
 *             |        |     |    |     |    |     |    |
 *             |        |     |    |     |    |     |    |
 *             |        | <-- |    |     |    |     |    |
 *             +--------+     +----+     +----+     +----+
 *
 * In the above graph, P is a parent node; A is an anchor node; C1, C2, and C3
 * are child nodes. A parent node has one reference at the 'first_child_offset'
 * slot pointing to the anchor node. An anchor node has two references: one at
 * the 'first_child_offset' slot pointing to the first child node in the list;
 * one at the 'parent_offset' slot pointing to the parent node. Each child node
 * has three references: one at the 'next_child_offset' slot pointing to the
 * next child in the list; one at the 'prev_child_offset' slot pointing to the
 * previous node in the list; one at the 'parent_offset' slot pointing to the
 * anchor node.
 *
 */
struct relationship_t
{
    gaia::common::gaia_type_t parent_type;

    gaia::common::gaia_type_t child_type;

    gaia::common::reference_offset_t first_child_offset;

    gaia::common::reference_offset_t next_child_offset;

    gaia::common::reference_offset_t prev_child_offset;

    gaia::common::reference_offset_t parent_offset;

    cardinality_t cardinality;

    bool parent_required;

    bool value_linked;
};

} // namespace db
} // namespace gaia
