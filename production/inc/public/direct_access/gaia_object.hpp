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

#include "gaia_base.hpp"
#include "auto_transaction.hpp"

using namespace std;
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

template<gaia::db::gaia_type_t T_gaia_type, 
    typename T_gaia, 
    typename T_fb, 
    typename T_obj,
    size_t N_references>
struct gaia_writer_t;

/**
 * The gaia_object_t that must be specialized to operate on a flatbuffer type.
 *
 * @tparam T_gaia_type an integer (gaia_type_t) uniquely identifying the flatbuffer table type
 * @tparam T_gaia the subclass type derived from this template
 * @tparam T_fb the flatbuffer table type to be implemented
 * @tparam T_obj the mutable flatbuffer type to be implemented
 * @tparam N_references the number of reference slots this type supports
 */
template<gaia::db::gaia_type_t T_gaia_type, 
    typename T_gaia, 
    typename T_fb, 
    typename T_obj,
    size_t N_references>
struct gaia_object_t : gaia_base_t
{
public:
    gaia_object_t() = delete;

    /**
     * Return a reference to a writer used to insert a row
     */
    static unique_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> writer();

    /**
     * Return a reference to a writer used to update a row
     */
    static unique_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> writer(const shared_ptr<T_gaia>& object);

    /**
     * Return a reference to a writer used to update a row
     */
    static unique_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> writer(gaia_id_t id);


    /**
     * This can be used for subscribing to rules when you don't
     * have a specific instance of the type.
     */
    static gaia::db::gaia_type_t s_gaia_type;

    /**
     * This can be used when you are passed a gaia_base_t
     * object and want to know the type at runtime.
     */
    gaia::db::gaia_type_t gaia_type() override { return T_gaia_type; }

    /**
     * Ask for the first object of a flatbuffer type, T_gaia_type.
     */
    static shared_ptr<T_gaia> get_first();

    /**
     * Ask for the next object of a flatbuffer type. This call must follow a call to the
     * static method get_first().
     */
    shared_ptr<T_gaia> get_next();

    /**
     * Ask for a specific object based on its id. References to this method must be qualified
     * by the T_gaia_type, and that type must match the type of the identified object.
     *
     * @param id the gaia_id_t of a specific storage engine object, of type T_gaia_type
     */
    static shared_ptr<T_gaia> get(gaia_id_t id);

    /**
     * Insert the values in this new object into a newly created storage engine object.
     * The user can get a new object by fetching the returned id using get(id)
     */
    static gaia_id_t insert_row(
        const unique_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>>& writer);

    /**
     * Insert the values in this new object into a newly created storage engine object.
     * The user can get a new object by fetching the returned id using get(id)
     */
    static void update_row(
        const unique_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>>& writer);

    /**
     * Delete the storage engine object. This doesn't destroy the extended data class
     * object.
     */
    void delete_row();

    /**
     * Delete the storage engine object specified by the id.
     */
    static void delete_row(gaia_id_t id);

    // Array of pointers to related objects.
    gaia_id_t* m_references;

protected:
    /**
     * This constructor supports creating new objects from existing
     * nodes in the database.  It is called by our get_object below.
     */
    gaia_object_t(gaia_id_t id, const char * gaia_typename);

     /**
     * Insert a mutable flatbuffer into a newly created storage engine object. This will be
     * used by the generated type-specific insert_row() method.
     */
    static gaia_id_t insert_row(flatbuffers::FlatBufferBuilder& fbb);

    // Flatbuffer referencing SE memory.
    const T_fb* m_fb;

private:
    static shared_ptr<T_gaia> get_object(gaia_ptr<gaia_se_node>& node_ptr);
    void refresh() override;
};

template<gaia::db::gaia_type_t T_gaia_type, 
    typename T_gaia, 
    typename T_fb, 
    typename T_obj,
    size_t N_references>
struct gaia_writer_t : public T_obj
{
private:
    gaia_writer_t() = default;
     

    // Gaia specific properties that are outside the flatbuffer payload 
    // definition but needed for row creation.
    struct gaia_t {
        gaia_id_t id;
    } m_gaia;

    // This class needs access to the private m_gaia information and private
    // constructor.
    friend gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>;
};

/*@}*/
} // direct_access
/*@}*/
} // gaia


// Pick up our template implementation.  These still
// need to be in the header so that template specializations
// that are declared later will pick up the definitions.
#include "gaia_object.inc"
