/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstring>
#include <cstdint>
#include <list>
#include <map>
#include "NullableString.h"
#include "cow_se.h"

using namespace std;
using namespace gaia_se;

//
// API outside of object instead of a base class
// might be attractive because it's more easily translatable
// to C
//
struct gaia_base
{
    static map<gaia_id_t, gaia_base *> s_gaia_cache;

    // pass through to storage engine
    static void begin_transaction() {gaia_se::begin_transaction();}
    static void commit_transaction() {gaia_se::commit_transaction();}
    static void rollback_transaction() {gaia_se::rollback_transaction();}

    virtual ~gaia_base() = default;
};
map<gaia_id_t, gaia_base *> gaia_base::s_gaia_cache;

// think about a non-template solution
template <gaia_se::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct gaia_obj : gaia_base
{
public:
    // virtual gaia_id_t Gaia_id() const = 0;

    virtual ~gaia_obj() { reset(); }

    gaia_obj() : _copy(nullptr), _fb(nullptr), _fbb(nullptr) {
        _id = get_next_id();
        s_gaia_cache[_id] = this;
    }

    #define get(field) (_copy ? (_copy->field) : (_fb->field()))
    // NOTE: either _fb or _copy should exist
    #define get_original(field) (_fb ? _fb->field() : _copy->field)
    #define get_str_original(field) (_fb ? _fb->field()->c_str() : _copy->field.c_str())
    #define get_str(field) (_copy ? _copy->field.c_str() : _fb->field() ? _fb->field()->c_str() : nullptr)
    #define set(field, value) (copy_write()->field = value)

    gaia_id_t get_next_id() {
        uuid_t uuid;
        gaia_id_t _node_id;
        uuid_generate(uuid);
        memcpy(&_node_id, uuid, sizeof(gaia_id_t));
        _node_id &= ~0x8000000000000000;
        return _node_id;
    }

    static T_gaia * GetFirst() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return GetObject(node_ptr);
    }

    static T_gaia * GetRowById(gaia_id_t id) {
        auto node_ptr = gaia_se_node::open(id);
        return GetObject(node_ptr);
    }

    // allow client to send in pointer (instead of always allocating)
    T_gaia * GetNext() {
        auto current_ptr = gaia_se_node::open(this->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        return GetObject(next_ptr);
    }

    gaia_id_t Gaia_id()
    {
        return _id;
    }

    void Insert()
    {
        // create the node
        // add to cache
        gaia_ptr<gaia_se_node> node_ptr;
        if (_copy != nullptr) {
            auto u = T_fb::Pack(*_fbb, _copy);
            _fbb->Finish(u);
            node_ptr = gaia_se_node::create(_id, T_gaia_type, _fbb->GetSize(), _fbb->GetBufferPointer());
            _fbb->Clear();
        } else {
            node_ptr = gaia_se_node::create(_id, T_gaia_type, 0, nullptr);
        }
        s_gaia_cache[node_ptr->id] = this;
        return;
    }

    void Update()
    {
        if (_copy) {
            // assert _fbb
            auto u = T_fb::Pack(*_fbb, _copy);
            _fbb->Finish(u);
            auto node_ptr = gaia_se_node::open(this->Gaia_id());
            node_ptr.update_payload(_fbb->GetSize(), _fbb->GetBufferPointer());
            _fbb->Clear();
            // the following code will point _fb at the new values
            // if this code isn't executed, then _fb can be used to access the original field values
            // reset();
            // _fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
        }
    } 

    void Delete()
    {
        auto node_ptr = gaia_se_node::open(_id);
        gaia_ptr<gaia_se_node>::remove(node_ptr);
        s_gaia_cache.erase(_id);
        reset();
    }

    // used by Airport
    T_obj * copy_write() {
        if (_copy == nullptr) {
            T_obj * copy = new T_obj();
            if (_fb)
                _fb->UnPackTo(copy);
            _copy = copy;
            _fbb = new flatbuffers::FlatBufferBuilder();
        }
        return _copy;
    }

protected:
    const T_fb * _fb; // flat buffer
    T_obj * _copy; // copy data changes
    flatbuffers::FlatBufferBuilder * _fbb;    // cached flat buffer builder for reuse
    gaia_id_t _id;

private:
    static T_gaia * GetObject(gaia_ptr<gaia_se_node>& node_ptr) 
    {
        T_gaia * obj = nullptr;
        if (node_ptr != nullptr) {
            auto it = s_gaia_cache.find(node_ptr->id);
            if (it != s_gaia_cache.end()) {
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia();
                s_gaia_cache.insert(pair<gaia_id_t, gaia_base *>(node_ptr->id, obj));
            }
            if (obj->_fb == nullptr) {
                auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
                obj->_fb = fb;
                obj->_id = node_ptr->id;
            }
        }
        return obj;
    }

    void reset()
    {
        if (_copy) {
            delete _copy;
        }
        if (_fbb) {
            delete _fbb;
        }
        _copy = nullptr;
        _fbb = nullptr;
    }

};
