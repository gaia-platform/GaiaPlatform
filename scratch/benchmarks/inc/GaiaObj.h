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
struct GaiaBase
{
    static map<gaia_id_t, GaiaBase *> s_gaia_cache;
    virtual ~GaiaBase() = default;
};
map<gaia_id_t, GaiaBase *> GaiaBase::s_gaia_cache;

// think about a non-template solution
template <gaia_se::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
struct GaiaObj : GaiaBase
{
public:
    virtual int64_t Gaia_id() const = 0;

    virtual ~GaiaObj() { reset(); }

    GaiaObj() : _copy(nullptr), _fb(nullptr), _fbb(nullptr) {}

    #define get(field) (_copy ? (_copy->field) : (_fb->field()))
    #define get_str(field) (_copy ? _copy->field.c_str() : _fb->field() ? _fb->field()->c_str() : nullptr)
    #define set(field, value) (copy_write()->field = value)

    static T_gaia * GetFirst() {
        auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
        return GetObject(node_ptr);
    }

    static T_gaia * GetRowById(gaia_id_t id) {
        return GetObject(id);
    }

    // allow client to send in pointer (instead of always allocating)
    T_gaia * GetNext() {
        auto current_ptr = gaia_se_node::open(this->Gaia_id());
        auto next_ptr = current_ptr.find_next();
        return GetObject(next_ptr);
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

    // used by Airport
    T_obj * copy_write() {
        if (_copy == nullptr) {
            T_obj * copy = new T_obj();
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
                s_gaia_cache.insert(pair<gaia_id_t, GaiaBase *>(node_ptr->id, obj));
            }
            if (obj->_fb == nullptr) {
                auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
                obj->_fb = fb;
            }
        }
        return obj;
    }

    static T_gaia * GetObject(gaia_id_t id)
    {
        T_gaia * obj = nullptr;
        if (id != 0) {
            auto it = s_gaia_cache.find(id);
            if (it != s_gaia_cache.end()) {
                obj = dynamic_cast<T_gaia *>(it->second);
            }
            else {
                obj = new T_gaia();
                s_gaia_cache.insert(pair<gaia_id_t, GaiaBase *>(id, obj));
            }
            if (obj->_fb == nullptr) {
                auto node_ptr = gaia_se_node::open(id);
                auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->payload);
                obj->_fb = fb;
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
