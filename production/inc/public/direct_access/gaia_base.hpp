/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>
#include <memory>

#include "gaia_common.hpp"
#include "gaia_exception.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia {

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access {

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

/**
 * The gaia_base_t struct is a tag to mark extended data class objects.
 */
struct gaia_base_t
{
    /**
     * The gaia_base_t and gaia_object_t shouldn't be instantiated directly. The
     * gaia_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    gaia_base_t() = delete;
    gaia_base_t(const char* gaia_typename) : m_typename(gaia_typename) {}

    const char* gaia_typename() { return m_typename; }
    virtual gaia_container_id_t container_id() = 0;
    virtual ~gaia_base_t() = default;

protected:
    // The name for this gaia container
    const char * m_typename;
};

// Exception when get() argument doesn't match the class type
class edc_invalid_object_type: public gaia_exception
{
public:
    edc_invalid_object_type(gaia_id_t id,
        gaia_container_id_t expected,
        const char* expected_typename,
        gaia_container_id_t actual);
};

// A child's parent pointer must match the parent record we have.
class edc_invalid_member: public gaia_exception
{
public:
    edc_invalid_member(
        gaia_id_t id,
        gaia_container_id_t parent,
        const char* parent_type,
        gaia_container_id_t child,
        const char* child_name);
};

// When a child refers to a parent, but is not found in that parent's list.
class edc_inconsistent_list: public gaia_exception
{
public:
    edc_inconsistent_list(
        gaia_id_t id,
        const char* parent_type,
        gaia_id_t child,
        const char* child_name);
};

// To connect two objects, a gaia_id() is needed but not available until SE create is called during
// the insert_row().
class edc_invalid_state: public gaia_exception
{
public:
    edc_invalid_state(
        const char* parent_type,
        const char* child_type);
};

// An attempt has been made to insert a member that has already been inserted somewhere.
class edc_already_inserted: public gaia_exception
{
public:
    edc_already_inserted(gaia_id_t parent, const char* parent_type);
};

/*@}*/
}
/*@}*/
}
