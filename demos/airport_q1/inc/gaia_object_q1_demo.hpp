/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// This file contains methods that are not supported in the current version of
// gaia_object.hpp. These methods navigate through SE edges using a paradigm
// that is not preferred moving forward.
//
// Do not use this file as the basis for any additional demos or benchmarks.

#pragma once

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include "flatbuffers/flatbuffers.h"
#include "gaia/nullable_string.hpp"
#include "storage_engine.hpp"

using namespace std;
using namespace gaia::db;

namespace gaia {
namespace common {

struct gaia_base_t
{
    // object caches
    // s_gaia_cache - Track every gaia_base_t object by the gaia_id. If the same
    //                gaia_id is accessed multiple times, this cache will find
    //                the same object containing any local transactional changes.
    //                Since each object may contain a pointer to the flatbuffer
    //                payload in the SE memory, or a local, mutable copy of the
    //                flatbuffer, these objects will be frequently referenced by
    //                their gaia_id and require quick access to their contents.
    // s_gaia_tx_cache - Track every gaia_base_t object that has been used in
    //                   the current transaction. Used to clear the field
    //                   values referenced in the objects at transaction commit
    //                   because they become stale. This separate cache is
    //                   maintained as a smaller subset of the s_gaia_cache so that
    //                   the whole cache doesn't have to be searched for contents
    //                   to be cleared. This map is cleared before every
    //                   transaction begins. By waiting until the next transaction
    //                   begins, program references to fields will not cause
    //                   crashes, even though the data is invalid.
    typedef map<gaia_id_t, gaia_base_t *> id_cache_t;
    static id_cache_t s_gaia_cache;
    static id_cache_t s_gaia_tx_cache;

    gaia_base_t() = delete;

    gaia_base_t(const char* gaia_typename)
    : m_id(0)
    , m_typename(gaia_typename)
    {
    }

    static void begin_transaction()
    {
        // The s_gaia_tx_cache is a list of objects containing stale data that
        // must be refreshed if a new transaction begins. Scan these objects to
        // clean out old values. The objects will not be deleted, as they will
        // continue to be tracked in the s_gaia_cache.
        for (auto it = s_gaia_tx_cache.begin(); it != s_gaia_tx_cache.end(); ++it)
        {
            it->second->reset(true);
        }
        s_gaia_tx_cache.clear();

        gaia::db::begin_transaction();
    }

    static void commit_transaction()   { gaia::db::commit_transaction(); }
    static void rollback_transaction() { gaia::db::rollback_transaction(); }

    gaia_id_t gaia_id() {
        return m_id;
    }
    
    const char* gaia_typename()
    {
        return m_typename;
    }

    virtual ~gaia_base_t() = default;

protected:
    gaia_base_t(gaia_id_t id, const char * gaia_typename)
    : m_id(id)
    , m_typename(gaia_typename)
    {
    }

    // The gaia_id assigned to this row.
    gaia_id_t m_id;
    // The typename for this gaia type
    const char * m_typename;

private:
    virtual void reset(bool) {}

};

// T_gaia_type - an integer (gaia_type_t) uniquely identifying the flatbuffer table type
// T_gaia      - the subclass type derived from this template
// T_fb        - the flatbuffer table type to be implemented
// T_obj       - the mutable flatbuffer type to be implemented
template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_object_t : gaia_base_t
{
public:
    enum class edge_orientation_t {
        to,
        from
    };

    virtual ~gaia_object_t()
    {
        s_gaia_cache.erase(m_id);
        s_gaia_tx_cache.erase(m_id);
        reset(true);
    }
    gaia_object_t() = delete;

    // This constructor supports completely new objects
    // that the database has not seen yet by creating
    // a copy buffer immediately.
    gaia_object_t(const char * gaia_typename) 
    : gaia_base_t(gaia_typename)
    , m_fb(nullptr)
    {
        copy_write();
    }

    #define GET_CURRENT(field) (m_copy ? (m_copy->field) : (m_fb ? m_fb->field() : 0))
    #define GET_ORIGINAL(field) (m_fb ? m_fb->field() : (m_copy ? m_copy->field : 0))
    #define GET_STR_ORIGINAL(field) (m_fb ? (m_fb->field() ? m_fb->field()->c_str() : nullptr) : (m_copy ? m_copy->field.c_str() : nullptr))
    #define GET_STR(field) (m_copy ? m_copy->field.c_str() : m_fb ? (m_fb->field()? m_fb->field()->c_str() : nullptr) : nullptr)
    #define SET(field, value) (copy_write()->field = value)

    static T_gaia* get_first() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return get_object<gaia_ptr<gaia_se_node>>(node_ptr);
    }

    T_gaia* get_next() {
        auto current_ptr = gaia_se_node::open(m_id);
        auto next_ptr = current_ptr.find_next();
        return get_object<gaia_ptr<gaia_se_node>>(next_ptr);
    }

    static T_gaia* get_row_by_id(gaia_id_t id) {
        auto node_ptr = gaia_se_node::open(id);
        return get_object<gaia_ptr<gaia_se_node>>(node_ptr);
    }

    static T_gaia* get_edge_by_id(gaia_id_t id) {
        auto node_ptr = gaia_se_edge::open(id);
        return get_object<gaia_ptr<gaia_se_edge>>(node_ptr);
    }

    T_gaia* get_first_node(gaia_type_t edge_type, edge_orientation_t edge_orientation) {
        auto node_ptr = gaia_se_node::open(m_id);
        gaia_ptr<gaia_se_edge> edge;
        if (edge_orientation == edge_orientation_t::to) {
            edge = node_ptr->next_edge_first;
            while (edge != nullptr && edge->type != edge_type) {
                edge = edge->next_edge_first;
            }
        }
        else {
            edge = node_ptr->next_edge_second;
            while (edge != nullptr && edge->type != edge_type) {
                edge = edge->next_edge_second;
            }
        }
        if (edge == nullptr) {
            m_current_edge = nullptr;
            return nullptr;
        }
        // There will be a node at the other end of this edge.
        auto next_node = edge_orientation == edge_orientation_t::to ? edge->node_second : edge->node_first;
        m_current_edge = edge;
        m_edge_orientation = edge_orientation;
        m_edge_type = edge_type;
        return get_object<gaia_ptr<gaia_se_node>>(next_node);
    }

    T_gaia* get_next_node() {
        if (m_current_edge == nullptr) {
            return nullptr;
        }
        if (m_edge_orientation == edge_orientation_t::to) {
            m_current_edge = m_current_edge->next_edge_first;
            while (m_current_edge != nullptr && m_current_edge->type != m_edge_type) {
                m_current_edge = m_current_edge->next_edge_first;
            }
        }
        else {
            m_current_edge = m_current_edge->next_edge_second;
            while (m_current_edge != nullptr && m_current_edge->type != m_edge_type) {
                m_current_edge = m_current_edge->next_edge_second;
            }
        }
        if (m_current_edge == nullptr) {
            return nullptr;
        }
        // there will be a node at the other end of this edge
        auto next_node = m_edge_orientation == edge_orientation_t::to ? m_current_edge->node_second : m_current_edge->node_first;
        return get_object<gaia_ptr<gaia_se_node>>(next_node);
    }

    gaia_id_t gaia_edge_id() {
        if (m_current_edge != nullptr) {
            return m_current_edge->id;
        }
        return 0;
    }

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
        s_gaia_cache[m_id] = this;
        s_gaia_tx_cache[m_id] = this;
        return;
    }

    void update_row()
    {
        if (m_copy) {
            auto node_ptr = gaia_se_node::open(m_id);
            if (nullptr == node_ptr) {
                throw invalid_node_id(m_id);
            }
            auto u = T_fb::Pack(*m_fbb, m_copy.get());
            m_fbb->Finish(u);
            node_ptr.update_payload(m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
        }
    }

    void delete_row()
    {
        auto node_ptr = gaia_se_node::open(m_id);
        if (nullptr == node_ptr) {
            throw invalid_node_id(m_id);
        }

        gaia_ptr<gaia_se_node>::remove(node_ptr);
        // A partial reset leaves m_fb alone. If program incorrectly references
        // fields in this deleted object, it will not crash.
        reset();
    }

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
    // Direction of edge traversal.
    edge_orientation_t m_edge_orientation;
    // Type of edge being traversed.
    gaia_type_t m_edge_type;
    // Current edge in a traversal.
    gaia_ptr<gaia_se_edge> m_current_edge;

    T_obj* copy_write()
    {
        if (m_copy == nullptr) {
            m_copy.reset(new T_obj());
            if (m_fb) {
                m_fb->UnPackTo(m_copy.get());
            }
            m_fbb.reset(new flatbuffers::FlatBufferBuilder());
        }
        return m_copy.get();
    }

private:
    template<typename T_se_object>
    static T_gaia* get_object(T_se_object& node_ptr)
    {
        T_gaia* obj = nullptr;
        if (node_ptr != nullptr) {
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()) {
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia(node_ptr->id);
                s_gaia_cache.insert(pair<gaia_id_t, gaia_base_t *>(node_ptr->id, obj));
            }
            if (obj->m_fb == nullptr) {
                auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
                obj->m_fb = fb;
                obj->m_id = node_ptr->id;
                s_gaia_tx_cache[obj->m_id] = obj;
            }
        }
        return obj;
    }

    void reset(bool clear_flatbuffer = false)
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
};
} // common
} // gaia
