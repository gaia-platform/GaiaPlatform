/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstring>

#include <list>

#include "flatbuffers/flatbuffers.h"

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/direct_access/edc_base.hpp"
#include "gaia/direct_access/nullable_string.hpp"

namespace gaia
{

/**
 * \addtogroup Gaia
 * @{
 */

namespace direct_access
{

/**
 * \addtogroup Direct
 * @{
 *
 * Implementation of Extended Data Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

template <gaia::common::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_writer_t;

/**
 * The gaia_object_t that must be specialized to operate on a flatbuffer type.
 *
 * @tparam T_gaia_type an integer (gaia_type_t) uniquely identifying the flatbuffer table type
 * @tparam T_gaia the subclass type derived from this template
 * @tparam T_fb the flatbuffer table type to be implemented
 * @tparam T_obj the mutable flatbuffer type to be implemented
 */
template <gaia::common::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_object_t : gaia_base_t
{
public:
    explicit gaia_object_t(const char* gaia_typename);

    /**
     * Return a reference that is pre-populated with values from the row
     */
    gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj> writer();

    /**
     * This can be used for subscribing to rules when you don't
     * have a specific instance of the type.
     */
    static gaia::common::gaia_type_t s_gaia_type;

    /**
     * This can be used when you are passed a gaia_base_t
     * object and want to know the type at runtime.
     */
    gaia::common::gaia_type_t gaia_type() override
    {
        return T_gaia_type;
    }

    /**
     * Ask for the first object of a flatbuffer type, T_gaia_type.
     */
    static T_gaia get_first();

    /**
     * Ask for the next object of a flatbuffer type. This call must follow a call to the
     * static method get_first().
     */
    T_gaia get_next();

    /**
     * Ask for a specific object based on its id. References to this method must be qualified
     * by the T_gaia_type, and that type must match the type of the identified object.
     *
     * @param id the gaia_id_t of a specific database object, of type T_gaia_type
     */
    static T_gaia get(gaia::common::gaia_id_t id);

    /**
     * Delete the database object. This doesn't destroy the extended data class
     * object.
     */
    void delete_row();

    /**
     * Delete the database object specified by the id.
     */
    static void delete_row(gaia::common::gaia_id_t id);

    /**
     * Get the array of pointers to related objects.
     */
    gaia::common::gaia_id_t* references() const override;

    /**
     * This is the database's identification of this object. The id can be
     * used to refer to this object later.
     */
    gaia::common::gaia_id_t gaia_id() const;

    /**
     * Returns true if there is an an underlying database object.
     */
    explicit operator bool() const;

    /**
     * Returns true if the gaia locator these objects represent
     * is the same.
     */
    bool operator==(const gaia_object_t& other) const;

protected:
    /**
     * This constructor supports creating new objects from existing
     * nodes in the database.  It is called by our get_object below.
     */
    gaia_object_t(gaia::common::gaia_id_t id, const char* gaia_typename);

    /**
     * Insert a mutable flatbuffer into a newly created database object. This will be
     * used by the generated type-specific insert_row() method.
     */
    static gaia::common::gaia_id_t insert_row(flatbuffers::FlatBufferBuilder& fbb);

    /**
     * Materialize the flatbuffer associated with this record
     */
    const T_fb* row() const;

    /**
     * Ensure the type requested by the gaia_id_t matches T_gaia_type. If the passed in
     * id does not exist in the database then return c_invalid_gaia_id.  If the id
     * does exist in the database and the type of the record matches then return the
     * passed in id.  If the type does not match then throw an edc_invalid_object_type
     * exception.
     */
    static gaia::common::gaia_id_t verify_type(gaia::common::gaia_id_t id);
};

template <gaia::common::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_writer_t : public T_obj, edc_db_t
{
    gaia_writer_t() = default;

    /**
     * Insert the values in this new object into a newly created database object.
     * The user can get a new object by fetching the returned id using get(id)
     */
    gaia::common::gaia_id_t insert_row();

    /**
     * Update the row values stored in this writer.
     */
    void update_row();

private:
    flatbuffers::FlatBufferBuilder m_builder;

    struct
    {
        gaia::common::gaia_id_t id;
    } m_gaia;

    friend gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>;
};

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia/direct_access/edc_object.inc"
