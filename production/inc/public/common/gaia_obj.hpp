/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include "flatbuffers/flatbuffers.h"
#include "nullable_string.hpp"
#include "se.hpp"

using namespace std;
using namespace gaia::db;

namespace gaia {
namespace common {

struct gaia_base_t
{
    typedef map<gaia_id_t, gaia_base_t *> ID_CACHE;
    static ID_CACHE s_gaia_cache;
    static ID_CACHE s_gaia_tx;

    static void begin_transaction()
    {
        // the first order of business is to clean out old values
        for (ID_CACHE::iterator it = s_gaia_tx.begin();
             it != s_gaia_tx.end();
             it++)
        {
            it->second->reset(true);
        }
        s_gaia_tx.clear();

        gaia::db::begin_transaction();
    }

    static void commit_transaction()   { gaia::db::commit_transaction(); }
    static void rollback_transaction() { gaia::db::rollback_transaction(); }

    virtual ~gaia_base_t() = default;

private:
    virtual void reset(bool) {}

};
gaia_base_t::ID_CACHE gaia_base_t::s_gaia_cache;
gaia_base_t::ID_CACHE gaia_base_t::s_gaia_tx;

// T_gaia_type - an integer (gaia_type_t) uniquely identifying the flatbuffer table type
// T_gaia      - the subclass type derived from this template
// T_fb        - the flatbuffer table type to be implemented
// T_obj       - the mutable flatbuffer type to be implemented
template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_obj_t : gaia_base_t
{
public:
    virtual ~gaia_obj_t() { 
        s_gaia_cache.erase(m_id);
        s_gaia_tx.erase(m_id);
        reset(true);
    }

    gaia_obj_t() : m_copy(nullptr), m_fb(nullptr), m_fbb(nullptr) {}

    #define get_current(field) (m_copy ? (m_copy->field) : (m_fb->field()))
    // NOTE: Either m_fb or m_copy should exist.
    #define get_original(field) (m_fb ? m_fb->field() : m_copy->field)
    #define get_str_original(field) (m_fb ? m_fb->field()->c_str() : m_copy->field.c_str())
    #define get_str(field) (m_copy ? m_copy->field.c_str() : m_fb->field() ? m_fb->field()->c_str() : nullptr)
    #define set(field, value) (copy_write()->field = value)

    static T_gaia* get_first() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return get_object(node_ptr);
    }

    T_gaia* get_next() {
        auto current_ptr = gaia_se_node::open(m_id);
        auto next_ptr = current_ptr.find_next();
        return get_object(next_ptr);
    }

    static T_gaia* get_row_by_id(gaia_id_t id) {
        auto node_ptr = gaia_se_node::open(id);
        return get_object(node_ptr);
    }

    gaia_id_t gaia_id() {
        return m_id;
    }

    void insert_row()
    {
        // Create the node and add to the cache.
        gaia_ptr<gaia_se_node> node_ptr;
        if (m_copy != nullptr) {
            auto u = T_fb::Pack(*m_fbb, m_copy);
            m_fbb->Finish(u);
            node_ptr = gaia_se_node::create(T_gaia_type, m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
        } else {
            node_ptr = gaia_se_node::create(T_gaia_type, 0, nullptr);
        }
        m_id = node_ptr->gaia_id();
        s_gaia_cache[m_id] = this;
        s_gaia_tx[m_id] = this;
        return;
    }

    void update_row()
    {
        if (m_copy) {
            // assert m_fbb
            auto u = T_fb::Pack(*m_fbb, m_copy);
            m_fbb->Finish(u);
            auto node_ptr = gaia_se_node::open(m_id);
            node_ptr.update_payload(m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
        }
    } 

    void delete_row()
    {
        auto node_ptr = gaia_se_node::open(m_id);
        gaia_ptr<gaia_se_node>::remove(node_ptr);
        reset(false);
    }

    T_obj* copy_write() {
        if (m_copy == nullptr) {
            T_obj* copy = new T_obj();
            if (m_fb)
                m_fb->UnPackTo(copy);
            m_copy = copy;
            m_fbb = new flatbuffers::FlatBufferBuilder();
        }
        return m_copy;
    }

protected:
    
    flatbuffers::FlatBufferBuilder* m_fbb; // cached flat buffer builder for reuse
    const T_fb* m_fb;  // flat buffer, referencing SE memory
    T_obj*    m_copy;  // private mutable flatbuffer copy of field changes
    gaia_id_t   m_id;  // gaia_id assigned to this row

private:
    static T_gaia* get_object(gaia_ptr<gaia_se_node>& node_ptr)
    {
        T_gaia* obj = nullptr;
        if (node_ptr != nullptr) {
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()) {
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia();
                s_gaia_cache.insert(pair<gaia_id_t, gaia_base_t *>(node_ptr->id, obj));
            }
            if (obj->m_fb == nullptr) {
                auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
                obj->m_fb = fb;
                obj->m_id = node_ptr->id;
                s_gaia_tx[obj->m_id] = obj;
            }
        }
        return obj;
    }

    void reset(bool full)
    {
        if (m_copy) {
            delete m_copy;
        }
        if (m_fbb) {
            delete m_fbb;
        }
        m_copy = nullptr;
        m_fbb = nullptr;
        if (full) {
            m_fb = nullptr;
        }
    }
};
} // common
} // gaia