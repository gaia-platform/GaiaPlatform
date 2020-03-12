/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <uuid/uuid.h>
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
    static map<gaia_id_t, gaia_base_t *> s_gaia_cache;

    virtual ~gaia_base_t() = default;
};
map<gaia_id_t, gaia_base_t *> gaia_base_t::s_gaia_cache;

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
        reset();
    }

    gaia_obj_t() : _copy(nullptr), m_fb(nullptr), m_fbb(nullptr) {
        m_id = get_next_id();
        s_gaia_cache[m_id] = this;
    }

    #define get_current(field) (_copy ? (_copy->field) : (m_fb->field()))
    // NOTE: Either m_fb or _copy should exist.
    #define get_original(field) (m_fb ? m_fb->field() : _copy->field)
    #define get_str_original(field) (m_fb ? m_fb->field()->c_str() : _copy->field.c_str())
    #define get_str(field) (_copy ? _copy->field.c_str() : m_fb->field() ? m_fb->field()->c_str() : nullptr)
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
        if (_copy != nullptr) {
            auto u = T_fb::Pack(*m_fbb, _copy);
            m_fbb->Finish(u);
            node_ptr = gaia_se_node::create(m_id, T_gaia_type, m_fbb->GetSize(), m_fbb->GetBufferPointer());
            m_fbb->Clear();
        } else {
            node_ptr = gaia_se_node::create(m_id, T_gaia_type, 0, nullptr);
        }
        return;
    }

    void update_row()
    {
        if (_copy) {
            // assert m_fbb
            auto u = T_fb::Pack(*m_fbb, _copy);
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
        reset();
    }

    T_obj* copy_write() {
        if (_copy == nullptr) {
            T_obj* copy = new T_obj();
            if (m_fb)
                m_fb->UnPackTo(copy);
            _copy = copy;
            m_fbb = new flatbuffers::FlatBufferBuilder();
        }
        return _copy;
    }

protected:
    
    const T_fb* m_fb; // flat buffer
    T_obj*    _copy; // copy data changes
    gaia_id_t   m_id; // The gaia_id assigned to the row.
    flatbuffers::FlatBufferBuilder* m_fbb; // cached flat buffer builder for reuse

private:
    gaia_id_t get_next_id() {
        uuid_t uuid;
        gaia_id_t _node_id;
        uuid_generate(uuid);
        memcpy(&_node_id, uuid, sizeof(gaia_id_t));
        _node_id &= ~0x8000000000000000;
        return _node_id;
    }

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
            }
        }
        return obj;
    }

    void reset()
    {
        if (_copy) {
            delete _copy;
        }
        if (m_fbb) {
            delete m_fbb;
        }
        _copy = nullptr;
        m_fbb = nullptr;
    }
};
} // common
} // gaia