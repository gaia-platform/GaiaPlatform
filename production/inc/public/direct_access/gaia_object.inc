/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////


// NOTE: This is included included by gaia_object.hpp as this is a template
// implementation file.  Because the template specializations of gaia_object_t are
// created by user-defined schema, we don't know what they will be apriori.  I have
// pulled them out of gaia_object.hpp, however, to include readability of the
// gaia_object_t declarations.
namespace gaia
{
namespace direct_access
{


/**
 * gaia_object_t implementation
 */

// Macros for strongly types field accessors used by
// gaia_object_t objects below.
#define GET_CURRENT(field) (m_copy ? (m_copy->field) : (m_fb->field()))
#define GET_ORIGINAL(field) (m_fb ? m_fb->field() : m_copy->field)
#define GET_STR_ORIGINAL(field) (m_fb ? (m_fb->field() ? m_fb->field()->c_str() : nullptr) : (m_copy ? m_copy->field.c_str() : nullptr))
#define GET_STR(field) (m_copy ? m_copy->field.c_str() : m_fb->field() ? m_fb->field()->c_str() : nullptr)
#define SET(field, value) (copy_write()->field = value)

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::gaia_object_t(const char * gaia_typename, size_t num_references)
: gaia_base_t(gaia_typename)
, m_num_references(num_references)
, m_references(nullptr)
, m_fb(nullptr)
{
    copy_write();
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::gaia_object_t(const char * gaia_typename)
    : gaia_object_t(gaia_typename, 0)
{
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::gaia_object_t(gaia_id_t id, const char * gaia_typename, size_t num_references) 
: gaia_base_t(id, gaia_typename)
, m_num_references(num_references)
, m_references(nullptr)
, m_fb(nullptr)
{
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::gaia_object_t(gaia_id_t id, const char * gaia_typename) 
: gaia_object_t(id, gaia_typename, 0)
{
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::~gaia_object_t()
{
    s_gaia_cache.erase(m_id);
    s_gaia_tx_cache.erase(m_id);
    reset(true);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_gaia* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::get_first()
{
    auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
    return get_object(node_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_gaia* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::get_next()
{
    auto current_ptr = gaia_se_node::open(m_id);
    auto next_ptr = current_ptr.find_next();
    return get_object(next_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_gaia* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::get_row_by_id(gaia_id_t id)
{
    auto node_ptr = gaia_se_node::open(id);
    return get_object(node_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::insert_row(size_t num_ptrs)
{
    // Create the node and add to the cache.
    gaia_ptr<gaia_se_node> node_ptr;

    if (0 == m_id) {
        m_id = gaia_se_node::generate_id();
    }
    if (m_copy) {
        auto u = T_fb::Pack(*m_fbb, m_copy.get());
        m_fbb->Finish(u);
        node_ptr = gaia_se_node::create(m_id, T_gaia_type, num_ptrs, m_fbb->GetSize(), m_fbb->GetBufferPointer());
        m_fbb->Clear();
        m_fb = flatbuffers::GetRoot<T_fb>(node_ptr->data());
    } else {
        // This situation only happens if an object representing
        // a deleted row is reused.  By giving the object a copy buffer, 
        // the object can be used to insert new values.
        m_fb = nullptr;
        copy_write();
        node_ptr = gaia_se_node::create(m_id, T_gaia_type, 0, 0, nullptr);
    }
    m_references = node_ptr->references();
    s_gaia_cache[m_id] = this;
    s_gaia_tx_cache[m_id] = this;
    return;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia_id_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::insert_row(flatbuffers::FlatBufferBuilder& fbb)
{
    gaia_id_t nodeId = gaia_se_node::generate_id();
    gaia_se_node::create(nodeId, T_gaia_type, 0, fbb.GetSize(), fbb.GetBufferPointer());
    return nodeId;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_gaia* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::insert_row(flatbuffers::FlatBufferBuilder& fbb, size_t num_ptrs)
{
    gaia_id_t node_id = gaia_se_node::generate_id();
    auto node_ptr = gaia_se_node::create(node_id, T_gaia_type, num_ptrs, fbb.GetSize(), fbb.GetBufferPointer());
    node_ptr->num_references = num_ptrs;
    return get_row_by_id(node_id);
}


template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::update_row()
{
    if (m_copy) {
        auto node_ptr = gaia_se_node::open(m_id);
        if (!node_ptr) {
            throw invalid_node_id(m_id);
        }
        auto u = T_fb::Pack(*m_fbb, m_copy.get());
        m_fbb->Finish(u);
        node_ptr.update_payload(m_fbb->GetSize(), m_fbb->GetBufferPointer());
        // The update_payload() method changes the physical location of the storage
        // engine object. Since m_references points into that object, it's necessary
        // to make sure it is pointing to the new one.
        m_references = node_ptr->references();
        m_fbb->Clear();
    }
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::delete_row()
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

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_obj* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::copy_write()
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

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
T_gaia* gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::get_object(gaia_ptr<gaia_se_node>& node_ptr)
{
    T_gaia* obj = nullptr;
    if (node_ptr) {
        auto it = s_gaia_cache.find(node_ptr->id);
        if (it != s_gaia_cache.end()) {
            if (it->second->gaia_type() != T_gaia::s_gaia_type) {
                // The T_gaia object will contain the type name we want for the exception.
                T_gaia expected;
                throw edc_invalid_object_type(node_ptr->id,
                    expected.gaia_type(),
                    expected.gaia_typename(),
                    it->second->gaia_type(),
                    it->second->gaia_typename());
            }
            obj = static_cast<T_gaia*>(it->second);
        }
        else {
            obj = new T_gaia(node_ptr->id);
            s_gaia_cache.insert(pair<gaia_id_t, gaia_base_t *>(node_ptr->id, obj));
        }
        if (!obj->m_fb) {
            obj->m_num_references = node_ptr->num_references;
            obj->m_references = node_ptr->references();
            auto fb = flatbuffers::GetRoot<T_fb>(node_ptr->data());
            obj->m_fb = fb;
            obj->m_id = node_ptr->id;
            s_gaia_tx_cache[obj->m_id] = obj;
        }
    }
    return obj;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::reset(bool clear_flatbuffer)
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

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj>
gaia::db::gaia_type_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj>::s_gaia_type = T_gaia_type;


} // direct_access
} // gaia
