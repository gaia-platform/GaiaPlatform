/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <flatbuffers/flatbuffers.h>

#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/direct_access/dac_array.hpp"
#include "gaia/direct_access/dac_base.hpp"
#include "gaia/direct_access/dac_expressions.hpp"
#include "gaia/direct_access/nullable_string.hpp"
#include "gaia/exceptions.hpp"

// Export all symbols declared in this file.
#pragma GCC visibility push(default)

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace direct_access
{
/**
 * \addtogroup direct_access
 * @{
 *
 * Implementation of Direct Access Classes. This provides a direct access API
 * for CRUD operations on the database.
 */

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_gaia, typename T_fb, typename T_obj>
class dac_writer_t;

/**
 * The dac_object_t that must be specialized to operate on a flatbuffer type.
 *
 * @tparam container_type_id an integer (gaia_type_t) uniquely identifying the flatbuffer table type
 * @tparam T_gaia the subclass type derived from this template
 * @tparam T_fb the flatbuffer table type to be implemented
 * @tparam T_obj the mutable flatbuffer type to be implemented
 */
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_gaia, typename T_fb, typename T_obj>
class dac_object_t : public dac_base_t
{
public:
    /**
     * This can be used for subscribing to rules when you don't
     * have a specific instance of the type.
     */
    static gaia::common::gaia_type_t s_gaia_type;

public:
    dac_object_t();

    /**
     * Return a reference that is pre-populated with values from the row
     */
    dac_writer_t<container_type_id, T_gaia, T_fb, T_obj> writer();

    /**
     * This can be used when you are passed a dac_base_t
     * object and want to know the type at runtime.
     */
    gaia::common::gaia_type_t gaia_type() override
    {
        return container_type_id;
    }

    /**
     * Ask for a specific object based on its id. References to this method must be qualified
     * by the container_type_id, and that type must match the type of the identified object.
     *
     * @param id the gaia_id_t of a specific database object, of type container_type_id
     */
    static T_gaia get(gaia::common::gaia_id_t id);

    /**
     * Delete the database object. This doesn't destroy the direct access class
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
     * Returns true if there is an an underlying database object.
     */
    explicit operator bool() const;

    /**
     * Returns true if the both objects have same identity.
     */
    bool operator==(const dac_object_t& other) const;

    /**
     * Returns true if the objects do not have have same identity.
     */
    bool operator!=(const dac_object_t& other) const;

protected:
    /**
     * This constructor supports creating new objects from existing
     * objects in the database.  It is called by our get_object below.
     */
    dac_object_t(gaia::common::gaia_id_t id);

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
     * Verify that the type requested by the gaia_id_t matches container_type_id.
     * If the passed in id does not exist in the database,
     * then throw an invalid_object_id exception.
     * If the type does not match container_type_id,
     * then throw an invalid_object_type exception.
     */
    static void verify_type(gaia::common::gaia_id_t id);

    /**
     * Convert a flatbuffers::Vector to the corresponding dac_vector_t.
     */
    template <typename T_type>
    static dac_vector_t<T_type> to_dac_vector(const flatbuffers::Vector<T_type>* vector_ptr)
    {
        return dac_vector_t<T_type>(vector_ptr);
    }
};

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_gaia, typename T_fb, typename T_obj>
class dac_writer_t : public T_obj, protected dac_db_t
{
    friend dac_object_t<container_type_id, T_gaia, T_fb, T_obj>;

public:
    dac_writer_t() = default;

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
};

// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia/direct_access/dac_object.inc"

/*@}*/
} // namespace direct_access
/*@}*/
} // namespace gaia

// Restore default hidden visibility for all symbols.
#pragma GCC visibility pop
