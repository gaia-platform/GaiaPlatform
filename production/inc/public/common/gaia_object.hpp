/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>
#include <cstdint>
#include <map>
#include "flatbuffers/flatbuffers.h"
#include "gaia_exception.hpp"
#include "nullable_string.hpp"
#include "storage_engine.hpp"

using namespace std;
using namespace gaia::db;

namespace gaia {

/**
 * \addtogroup Gaia
 * @{
 */

namespace common {

/**
 * \addtogroup Common
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
struct gaia_base_t;
typedef shared_ptr<gaia_base_t> db_ptr;
typedef weak_ptr<gaia_base_t> obj_ptr;

struct gaia_base_t
{
    typedef map<gaia_id_t, db_ptr> id_cache_t;
    typedef map<gaia_base_t*, obj_ptr> obj_cache_t;

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
     * Track new objects that have been created but not yet inserted
     * into the database.  These get moved into the s_gaia_cache on insertion.
     * This cache will not hold the references alive.
     */
    static obj_cache_t s_obj_cache;

    /**
     * Install a begin_hook the first time any gaia object is instantiated.
     */ 
    static bool s_tx_hooks_installed;

    gaia_base_t() = delete;

    gaia_base_t(const char* gaia_typename)
    : gaia_base_t(0, gaia_typename)
    {
    }

    /**
     * This is the storage engine's identification of this object. The id can be
     * used to refer to this object later.
     */
    gaia_id_t gaia_id() 
    {
        return m_id;
    }

    /**
     * The gaia_base_t and gaia_object_t shouldn't be instantiated directly. The
     * gaia_object_t is created to be subclassed by a "typed" class that is identified
     * with a flatbuffer table. This method returns the name of that type.
     */
    const char* gaia_typename()
    {
        return m_typename;
    }

    /**
     * The s_gaia_tx_cache is a list of objects containing stale data that
     * must be refreshed if a new transaction begins. Scan these objects to
     * clean out old values. The objects will not be deleted, as they will
     * continue to be tracked in the s_gaia_cache.
     * 
     * This commit_hook() must be used together with the extended data
     * class objects.  It is executed after the transaction has been committed.
     */    
    static void commit_hook()
    {
    }

    static void pre_commit()
    {
        for (auto it = s_gaia_cache.begin(); it != s_gaia_cache.end(); ++it)
        {
            db_ptr& obj = it->second;

            // Only do the operation if there is an
            // outstanding reference besides this cache
            // object reference.
            if (!obj.unique())
            {
                obj->do_operation();
            }
        }

        for (auto it = s_obj_cache.begin(); it != s_obj_cache.end(); ++it) 
        {
            obj_ptr& ptr = it->second;
            if (!ptr.expired())
            {
                ptr.lock()->do_operation();
            }
        }
    }

    static void begin_hook() 
    {
        for (auto it = s_gaia_cache.begin(); it != s_gaia_cache.end();)
        {
            db_ptr& obj = it->second;
            // If there is an outstanding reference to this object then
            // drop it from the cache.  Otherwise, refresh the object so
            // that it picks up any changes from another comitted
            // transaction.
            if (obj.unique())
            {
                it = s_gaia_cache.erase(it);
            }
            else
            {
                obj->reset(true);
                obj->refresh();
                ++it;
            }
        }

        // clean up object cache as well
        for (auto it = s_obj_cache.begin(); it != s_obj_cache.end();)
        {
            obj_ptr& obj = it->second;
            if (obj.expired())
            {
                it = s_obj_cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    static void rollback_hook() {}

    virtual gaia_type_t gaia_type() = 0;
    virtual ~gaia_base_t() = default;

protected:
    gaia_base_t(gaia_id_t id, const char * gaia_typename)
    : m_id(id)
    , m_typename(gaia_typename)
    {
        set_tx_hooks();
    }

    // The first time we put any gaia object in our cache
    // we should install our commit hook.
    static void set_tx_hooks()
    {
        if (!s_tx_hooks_installed)
        {
            // Do not overwrite an already established hook.  This could happen
            // if an application has subscribed to transaction events.
            if (!gaia::db::s_tx_begin_hook)
            {
                gaia::db::s_tx_begin_hook = begin_hook;
            }
            s_tx_hooks_installed = true;
        }
    }

    // The gaia_id assigned to this row.
    gaia_id_t m_id;
    // The typename for this gaia type
    const char * m_typename;

private:
    virtual void reset(bool) = 0;
    virtual void refresh() = 0;
    virtual void do_operation() {};
};

// Exception when get_row_by_id() argument doesn't match the class type
class edc_invalid_object_type: public gaia_exception
{
public:
    edc_invalid_object_type(gaia_id_t id, gaia_type_t expected, const char* expected_type,
        gaia_type_t actual, const char* type_name) {
        stringstream msg;
        msg << "requesting Gaia type " << expected_type << "(" << expected << ") but object identified by "
            << id << " is type " << type_name << "(" << actual << ")";
        m_message = msg.str();
    }
};

class auto_transaction_t
{
public:
    auto_transaction_t()
    {
        gaia::db::begin_transaction();
    }

    void commit(bool auto_begin = true)
    {
        gaia_base_t::pre_commit();
        gaia::db::commit_transaction();
        if (auto_begin)
        {
            gaia::db::begin_transaction();
        }
    }

    ~auto_transaction_t()
    {
        if (gaia::db::gaia_mem_base::is_tx_active())
        {
            gaia::db::rollback_transaction();
        }
    }
};

// Macros for strongly types field accessors used by
// gaia_object_t objects below.
#define GET_CURRENT(field) (m_copy ? (m_copy->field) : (m_fb->field()))
#define GET_ORIGINAL(field) (m_fb ? m_fb->field() : m_copy->field)
#define GET_STR_ORIGINAL(field) (m_fb ? (m_fb->field() ? m_fb->field()->c_str() : nullptr) : (m_copy ? m_copy->field.c_str() : nullptr))
#define GET_STR(field) (m_copy ? m_copy->field.c_str() : m_fb->field() ? m_fb->field()->c_str() : nullptr)
#define SET(field, value) (copy_write()->field = value)

/**
 * The gaia_object_t that must be specialized to operate on a flatbuffer type.
 * 
 * @tparam T_gaia_type an integer (gaia_type_t) uniquely identifying the flatbuffer table type
 * @tparam T_gaia the subclass type derived from this template
 * @tparam T_fb the flatbuffer table type to be implemented
 * @tparam T_obj the mutable flatbuffer type to be implemented
 */
template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_object_t : gaia_base_t
{
public:
    virtual ~gaia_object_t()
    {
        reset(true);
    }
    gaia_object_t() = delete;

    /**
     * This constructor supports completely new objects that the database has not seen yet
     * by creating a copy buffer immediately.
     */
    gaia_object_t(const char * gaia_typename) 
    : gaia_base_t(gaia_typename)
    , m_fb(nullptr)
    {
        copy_write();
    }

    /**
     * This can be used for subscribing to rules when you don't
     * have a specific instance of the type.
     */ 
    static gaia::db::gaia_type_t s_gaia_type;

    /**
     * This can be used when you are passed a gaia_base_t
     * object and want to know the type at runtime.
     */ 
    gaia::db::gaia_type_t gaia_type() override
    {
        return T_gaia_type;
    }

    /**
     * Factory method for creating new T_gaia objects
     * not bound to the database.
     */
    static shared_ptr<T_gaia> create()
    {
        shared_ptr<T_gaia> client_ptr(new T_gaia());
        gaia_base_t::s_obj_cache.insert(
            pair<gaia_base_t*, obj_ptr>(client_ptr.get(), client_ptr));
        return client_ptr;
    }

    /**
     * Ask for the first object of a flatbuffer type, T_gaia_type.
     */
    static shared_ptr<T_gaia> get_first() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return get_object(node_ptr);
    }

    /**
     * Ask for the next object of a flatbuffer type. This call must follow a call to the
     * static method get_first().
     */
    shared_ptr<T_gaia> get_next() {
        auto current_ptr = gaia_se_node::open(m_id);
        auto next_ptr = current_ptr.find_next();
        return get_object(next_ptr);
    }

    /**
     * Ask for a specific object based on its id. References to this method must be qualified
     * by the T_gaia_type, and that type must match the type of the identified object.
     * 
     * @param id the gaia_id_t of a specific storage engine object, of type T_gaia_type
     */
    static shared_ptr<T_gaia> get_row_by_id(gaia_id_t id) {
        auto node_ptr = gaia_se_node::open(id);
        return get_object(node_ptr);
    }

    /**
     * Insert the mutable flatbuffer contents contained in this new object into a newly
     * created storage engine object. The member m_copy contains field values that have
     * been set prior to this insert_row(). After the storage engine object exists, it
     * must be modified only through the update_row() method.
     */
    void insert_row()
    {
        // Create the node and add to the cache.
        gaia_ptr<gaia_se_node> node_ptr;

        if (0 == m_id) {
            m_id = gaia_se_node::generate_id();
        }
        if (m_copy) {
            auto u = T_fb::Pack(*m_fbb, m_copy.get());
            m_fbb->Finish(u);
            node_ptr = gaia_se_node::create(m_id, T_gaia_type, m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
            m_fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
        } else {
            // This situation only happens if an object representing
            // a deleted row is reused.  By giving the object a copy buffer, 
            // the object can be used to insert new values.
            copy_write();
            node_ptr = gaia_se_node::create(m_id, T_gaia_type, 0, nullptr);
        }

        // Move the object from the new object cache to the db cache
        auto it = s_obj_cache.find(this);
        if (it != s_obj_cache.end())
        {
            obj_ptr& ptr = it->second;
            if (!ptr.expired())
            {
                s_gaia_cache.insert(pair<gaia_id_t, db_ptr>(m_id, ptr.lock()));
            }
            s_obj_cache.erase(it);
        }
    }

    /**
     * Write the mutable flatbuffer values to the storage engine. This involves the creation
     * of a new storage engine object because the existing object cannot be modified. The new
     * storage engine object will be addressed by the gaia_id_t m_id.
     */
    void update_row()
    {
        if (m_copy) {
            auto node_ptr = gaia_se_node::open(m_id);
            if (!node_ptr) {
                throw invalid_node_id(m_id);
            }
            auto u = T_fb::Pack(*m_fbb, m_copy.get());
            m_fbb->Finish(u);
            node_ptr.update_payload(m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
        }
    }

    /**
     * Delete the storage engine object. This doesn't destroy the extended data class
     * object, which is owned by the program.
     */
    void delete_row()
    {
        auto node_ptr = gaia_se_node::open(m_id);
        if (!node_ptr) {
            throw invalid_node_id(m_id);
        }

        gaia_ptr<gaia_se_node>::remove(node_ptr);
        // A partial reset leaves m_fb alone. If program incorrectly references
        // fields in this deleted object, it will not crash.
        reset();
    }

    /**
     * Insert a mutable flatbuffer into a newly created storage engine object. This will be
     * used by the generated type-specific insert_row() method.
     */
    static gaia_id_t insert_row(flatbuffers::FlatBufferBuilder& fbb)
    {
        gaia_id_t nodeId = gaia_se_node::generate_id();
        gaia_se_node::create(nodeId, T_gaia_type, fbb.GetSize(), fbb.GetBufferPointer());
        return nodeId;
    }

protected:
    // This constructor supports creating new objects from existing
    // nodes in the database.  It is called by our get_object below.
    gaia_object_t(gaia_id_t id, const char * gaia_typename) 
    : gaia_base_t(id, gaia_typename)
    , m_fb(nullptr)
    {
    }

    // Cached flatbuffer builder for reuse when inserting
    // or modifying rows.
    unique_ptr<flatbuffers::FlatBufferBuilder> m_fbb; 
    // Mutable flatbuffer copy of field changes.
    unique_ptr<T_obj> m_copy;   
    // Flatbuffer referencing SE memory.
    const T_fb* m_fb;

    /**
     * Create the mutable flatbuffer object (m_copy) if it doesn't exist as a member
     * yet. If this points to an existing storage engine object, load the current
     * field values into m_copy.
     */
    T_obj* copy_write()
    {
        if (!m_copy) {
            m_copy.reset(new T_obj());
            if (m_fb) {
                m_fb->UnPackTo(m_copy.get());
            }
            m_fbb.reset(new flatbuffers::FlatBufferBuilder());
        }
        return m_copy.get();
    }

private:
    static shared_ptr<T_gaia> get_object(gaia_ptr<gaia_se_node>& node_ptr)
    {
        shared_ptr<T_gaia> client_ptr;
        if (node_ptr) {
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()) {
                client_ptr = dynamic_pointer_cast<T_gaia>(it->second);
                if (!client_ptr) {
                    // The T_gaia object will contain the type name we want for the exception.
                    T_gaia expected;
                    auto actual = it->second;
                    throw edc_invalid_object_type(node_ptr->id,
                        expected.gaia_type(),
                        expected.gaia_typename(),
                        actual->gaia_type(),
                        actual->gaia_typename());
                }
            }
            else {
                client_ptr.reset(new T_gaia(node_ptr->id));
                s_gaia_cache.insert(pair<gaia_id_t, db_ptr>(node_ptr->id, client_ptr));
            }
            if (!client_ptr->m_fb) {
                client_ptr->m_id = node_ptr->id;
                client_ptr->refresh();
            }
        }
        return client_ptr;
    }

    void reset(bool clear_flatbuffer = false) override
    {
        m_copy.reset();
        m_fbb.reset();

        // A full reset clears m_fb so that it will be read afresh the next
        // time the object is located.  We do not own the flatbuffer so
        // don't delete it.
        if (clear_flatbuffer) {
            m_fb = nullptr;
        }
    }

    void refresh() override
    {
        if (m_id)
        {
            auto node_ptr = gaia_se_node::open(m_id);
            if (node_ptr) {
                m_fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
            }
        }
    }

    void do_operation() override
    {
        if (m_copy)
        {
            if (m_id == 0)
            {
                insert_row();
            }
            else
            {
                update_row();
            }
        }
    }
};
/*@}*/
// Static definition of the gaia_type in gaia_object_t class above.
template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
    gaia::db::gaia_type_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::s_gaia_type = T_gaia_type;
} // common
/*@}*/
} // gaia
