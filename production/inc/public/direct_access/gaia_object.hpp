/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>
#include <cstdint>
#include <list>
#include "flatbuffers/flatbuffers.h"
#include "nullable_string.hpp"

#include "gaia_common.hpp"
#include "gaia_base.hpp"
#include "gaia_ptr.hpp"

using namespace std;
using namespace gaia::common;
using namespace gaia::db;

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
 * The gaia_object_t that must be specialized to operate on a flatbuffer type.
 *
 * @tparam T_gaia_type an integer (gaia_type_t) uniquely identifying the flatbuffer table type
 * @tparam T_gaia the subclass type derived from this template
 * @tparam T_fb the flatbuffer table type to be implemented
 * @tparam T_obj the mutable flatbuffer type to be implemented
 */
template <gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_object_t : gaia_base_t
{
public:
    gaia_object_t() = delete;
    virtual ~gaia_object_t();

    /**
     * This constructor supports completely new objects that the database has not seen yet
     * by creating a copy buffer immediately.
     */
    gaia_object_t(const char * gaia_typename, size_t num_references);

    /**
     * Needed for compatibility with earlier generated headers. May become obsolete.
     */
    gaia_object_t(const char * gaia_typename);

    /**
     * This can be used for subscribing to rules when you don't
     * have a specific instance of the type.
     */
    static gaia_type_t s_gaia_type;

    /**
     * This can be used when you are passed a gaia_base_t
     * object and want to know the type at runtime.
     */
    gaia_type_t gaia_type() override { return T_gaia_type; }

    /**
     * Ask for the first object of a flatbuffer type, T_gaia_type.
     */
    static T_gaia* get_first();

    /**
     * Ask for the next object of a flatbuffer type. This call must follow a call to the
     * static method get_first().
     */
    T_gaia* get_next();

    /**
     * Ask for a specific object based on its id. References to this method must be qualified
     * by the T_gaia_type, and that type must match the type of the identified object.
     *
     * @param id the gaia_id_t of a specific storage engine object, of type T_gaia_type
     */
    static T_gaia* get_row_by_id(gaia_id_t id);

    /**
     * Insert the mutable flatbuffer contents contained in this new object into a newly
     * created storage engine object. The member m_copy contains field values that have
     * been set prior to this insert_row(). After the storage engine object exists, it
     * must be modified only through the update_row() method.
     */
    void insert_row(size_t num_ptrs);

    void insert_row() { insert_row(0); }


    /**
     * Insert a mutable flatbuffer into a newly created storage engine object. This will be
     * used by the generated type-specific insert_row() method.
     */
    static T_gaia* insert_row(flatbuffers::FlatBufferBuilder& fbb, size_t num_ptrs);

    static gaia_id_t insert_row(flatbuffers::FlatBufferBuilder& fbb);

    /**
     * Write the mutable flatbuffer values to the storage engine. This involves the creation
     * of a new storage engine object because the existing object cannot be modified. The new
     * storage engine object will be addressed by the gaia_id_t m_id.
     */
    void update_row();

    /**
     * Delete the storage engine object. This doesn't destroy the extended data class
     * object, which is owned by the program.
     */
    void delete_row();

    // Array of pointers to related objects.
    size_t m_num_references;
    gaia_id_t* m_references;

protected:
    /**
     * This constructor supports creating new objects from existing
     * nodes in the database.  It is called by our get_object below.
     */
    gaia_object_t(gaia_id_t id, const char * gaia_typename, size_t num_references);

    // For compatibility with old generated headers. May become obsolete.
    gaia_object_t(gaia_id_t id, const char * gaia_typename);

    /**
     * Create the mutable flatbuffer object (m_copy) if it doesn't exist as a member
     * yet. If this points to an existing storage engine object, load the current
     * field values into m_copy.
     */
    T_obj* copy_write();

    // Cached flatbuffer builder for reuse when inserting
    // or modifying rows.
    unique_ptr<flatbuffers::FlatBufferBuilder> m_fbb;
    // Mutable flatbuffer copy of field changes.
    unique_ptr<T_obj> m_copy;
    // Flatbuffer referencing SE memory.
    const T_fb* m_fb;

private:
    static T_gaia* get_object(gaia_ptr& node_ptr);
    void reset(bool clear_flatbuffer = false) override;
};

/*@}*/
} // direct_access
/*@}*/
} // gaia


// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia_object.inc"
