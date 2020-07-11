/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <map>

#include "gaia_exception.hpp"
#include "storage_engine.hpp"

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

/**
 * The gaia_base_t struct provides control over the extended data class objects by
 * keeping a cached pointer to each storage engine object that has been accessed.
 * Storage engine objects are identified by the gaia_id_t (currently a 64-bit
 * integer). When the same object is referenced multiple times, the cached
 * gaia object associated with the gaia_id_t will be used again.
 *
 * A second cache is maintained to track objects that have been involved in a
 * transaction. These objects (which may be small in number compared to the
 * complete cache of objects) will be "cleared" at the beginning of each new
 * transaction. This ensures that any changes made by other transactions will be
 * refreshed if they are accessed again.
 */
struct gaia_base_t
{
    typedef map<gaia_id_t, gaia_base_t *> id_cache_t;
    /**
     * Track every gaia_base_t object by the gaia_id_t. If the same gaia_id_t is
     * accessed multiple times, this cache will find the same object containing
     * any local transactional changes. Since each object may contain a pointer
     * to the flatbuffer payload in the SE memory, or a local, mutable copy of the
     * flatbuffer, these objects will be frequently referenced by their gaia_id and
     * require quick access to their contents.
     */
    static id_cache_t s_gaia_cache;
    /**
     * Track every gaia_base_t object that has been used in the current transaction.
     * Used to clear the field values referenced in the objects at transaction commit
     * because they become stale. This separate cache is maintained as a smaller
     * subset of the s_gaia_cache so that the whole cache doesn't have to be searched
     * for contents to be cleared. This map is cleared before every transaction begins.
     * By waiting until the next transaction begins, program references to fields will
     * not cause crashes, even though the data is invalid.
     */
    static id_cache_t s_gaia_tx_cache;
    /**
     * Install a commit_hook the first time any gaia object is instantiated.
     */
    static bool s_tx_hooks_installed;

    gaia_base_t() = delete;

    gaia_base_t(const char* gaia_typename);

    /**
     * This is the storage engine's identification of this object. The id can be
     * used to refer to this object later.
     */
    gaia_id_t gaia_id() { return m_id; }

    /**
     * The gaia_base_t and gaia_object_t shouldn't be instantiated directly. The
     * gaia_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    const char* gaia_typename() { return m_typename; }

    /**
     * The s_gaia_tx_cache is a list of objects containing stale data that
     * must be refreshed if a new transaction begins. Scan these objects to
     * clean out old values. The objects will not be deleted, as they will
     * continue to be tracked in the s_gaia_cache.
     *
     * This commit_hook() must be used together with the extended data
     * class objects.  It is executed after the transaction has been committed.
     */
    static void commit_hook();

    /**
     * These hooks are intentionally left empty. They are provided
     * so that the rule subscriber can dumbly generate calls to them
     * when generating its own hooks to generate transaction events.
     * If code is run as part of begin or rollback hooks then do not
     * forget to set the hook in set_tx_hooks below.
     */
    static void begin_hook() {}
    static void rollback_hook() {}

    virtual gaia_type_t gaia_type() = 0;
    virtual ~gaia_base_t() = default;

protected:
    gaia_base_t(gaia_id_t id, const char * gaia_typename);

    // The first time we put any gaia object in our cache
    // we should install our commit hook.
    static void set_tx_hooks();

    // The gaia_id assigned to this row.
    gaia_id_t m_id;
    // The typename for this gaia type
    const char * m_typename;

private:
    virtual void reset(bool) = 0;
};

// Exception when get_row_by_id() argument doesn't match the class type
class edc_invalid_object_type: public gaia_exception
{
public:
    edc_invalid_object_type(gaia_id_t id, 
        gaia_type_t expected, 
        const char* expected_type,
        gaia_type_t actual, 
        const char* type_name);
};

// A child's parent pointer must match the parent record we have.
class edc_invalid_member: public gaia_exception
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
class edc_unstored_row: public gaia_exception
{
public:
    edc_unstored_row(
        const char* parent_type, 
        const char* child_type);
};

/*@}*/
}
/*@}*/
}
