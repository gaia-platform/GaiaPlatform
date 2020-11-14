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

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace db
{
class gaia_ptr;
}

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

/**
 * The gaia_base_t struct is a tag to mark extended data class objects as well as provide
 * non-template functionality.
 */
struct gaia_base_t
{
    /**
     * The gaia_base_t and gaia_object_t shouldn't be instantiated directly. The
     * gaia_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    gaia_base_t() = delete;
    gaia_base_t(const char* gaia_typename);
    gaia_base_t(const char* gaia_typename, gaia_id_t id);
    gaia_base_t(const gaia_base_t& other);
    gaia_base_t& operator=(const gaia_base_t& other);

    const char* gaia_typename()
    {
        return m_typename;
    }
    virtual gaia_type_t gaia_type() = 0;
    virtual ~gaia_base_t() = default;

    // helpers
    static gaia_id_t find_first(gaia_type_t container);
    static gaia_id_t find_next(gaia_id_t id);
    static gaia_id_t get_reference(gaia_id_t id, size_t slot);
    static gaia_id_t insert(gaia_type_t container, size_t num_refs, size_t data_size, const void* data);
    static void update(gaia_id_t id, size_t data_size, const void* data);
    static bool insert_reference(
        gaia_id_t parent_id, gaia_id_t child_id, size_t parent_slot, size_t child_slot, size_t next_child_slot);
    static bool remove_reference(
        gaia_id_t parent_id, gaia_id_t child_id, size_t parent_slot, size_t child_slot, size_t next_child_slot,
        bool& invalid_member);

protected:
    bool exists() const;
    bool equals(const gaia_base_t& other) const;

    static void delete_row(gaia_id_t id);
    static bool exists(gaia_id_t id, gaia_type_t& type);
    gaia_id_t id() const;

    const char* data() const;
    gaia_id_t* references();
    gaia_id_t find_next();

    // The typename for this gaia type
    const char* m_typename;

    // Opaque record structure
    gaia_handle_t m_record;
    gaia::db::gaia_ptr* m_record_ptr;
};

// Exception when get() argument does not match the class type.
class edc_invalid_object_type : public gaia_exception
{
public:
    edc_invalid_object_type(gaia_id_t id, gaia_type_t expected_type, const char* expected_typename, gaia_type_t actual_type);
};

// A child's parent pointer must match the parent record we have.
class edc_invalid_member : public gaia_exception
{
public:
    edc_invalid_member(
        gaia_id_t id,
        gaia_type_t parent,
        const char* parent_type,
        gaia_type_t child,
        const char* child_name);
};

// When a child refers to a parent, but is not found in that parent's list.
class edc_inconsistent_list : public gaia_exception
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
class edc_invalid_state : public gaia_exception
{
public:
    edc_invalid_state(
        const char* parent_type,
        const char* child_type);
};

// An attempt has been made to insert a member that has already been inserted somewhere.
class edc_already_inserted : public gaia_exception
{
public:
    edc_already_inserted(gaia_id_t parent, const char* parent_type);
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia
