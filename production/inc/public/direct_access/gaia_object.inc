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
#define GET(field) (m_fb->field())
#define GET_STR(field) (m_fb->field() ? m_fb->field()->c_str() : nullptr)

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::gaia_object_t(gaia_id_t id, const char * gaia_typename) 
    : gaia_base_t(id, gaia_typename)
    , m_references(nullptr)
    , m_fb(nullptr)
{
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> 
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::create_writer()
{
    shared_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> ptr;
    ptr.reset(new gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>());
    return ptr;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>> 
gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::writer()
{
    // If we haven't created a writer for this object before or have no outstanding references
    // then hand out a new one
    if (m_writer)
    {
        return m_writer;
    }

    // Create a new writer and copy the flatbuffer values to the writer.
    m_writer.reset(new gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>());
    m_fb->UnPackTo(m_writer.get());

    return m_writer;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<T_gaia> gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::get_first()
{
    auto node_ptr = gaia_ptr<gaia_se_node>::find_first(T_gaia_type);
    return get_object(node_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<T_gaia> gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::get_next()
{
    auto current_ptr = gaia_se_node::open(m_id);
    auto next_ptr = current_ptr.find_next();
    return get_object(next_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<T_gaia> gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::get(gaia_id_t id)
{
    auto node_ptr = gaia_se_node::open(id);
    return get_object(node_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
gaia_id_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::insert_row(
    const shared_ptr<gaia_writer_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>>& writer)
{
    gaia_ptr<gaia_se_node> node_ptr;
    flatbuffers::FlatBufferBuilder& builder = writer->m_builder;

    gaia_id_t id = gaia_se_node::generate_id();
    auto u = T_fb::Pack(builder, writer.get());
    builder.Finish(u);
    node_ptr = gaia_se_node::create(id, T_gaia_type, N_references, builder.GetSize(), 
        builder.GetBufferPointer());
    builder.Clear();

    return id;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
gaia_id_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::insert_row(flatbuffers::FlatBufferBuilder& fbb)
{
    gaia_id_t node_id = gaia_se_node::generate_id();
    gaia_se_node::create(node_id, T_gaia_type, N_references, fbb.GetSize(), fbb.GetBufferPointer());
    return node_id;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::update_row()
{
    if (!m_writer)
    {
        return;
    }

    auto node_ptr = gaia_se_node::open(m_id);
    if (!node_ptr)
    {
        throw invalid_node_id(m_id);
    }

    // Update the value in the storage engine.
    flatbuffers::FlatBufferBuilder& builder = m_writer->m_builder;
    auto u = T_fb::Pack(builder, m_writer.get());
    builder.Finish(u);
    node_ptr.update_payload(builder.GetSize(), builder.GetBufferPointer());
    builder.Clear();

    // Readback the new value.
    refresh(node_ptr);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::delete_row()
{
    delete_row(m_id);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::delete_row(gaia_id_t id)
{
    auto node_ptr = gaia_se_node::open(id);
    if (!node_ptr) 
    {
        throw invalid_node_id(id);
    }

    gaia_ptr<gaia_se_node>::remove(node_ptr);

    // Remove the object from the gaia cache so that we don't try to update
    // an invalided flatbuffer.  This allows the object to be still
    // accessible for any outstanding references.
    s_gaia_cache.erase(id);
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
shared_ptr<T_gaia> gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::get_object(gaia_ptr<gaia_se_node>& node_ptr)
{
    shared_ptr<T_gaia> client_ptr;
    if (!node_ptr)
    {
        return client_ptr;
    }
    auto it = s_gaia_cache.find(node_ptr->id);
    if (it != s_gaia_cache.end()) 
    {
        if (it->second->gaia_type() != T_gaia::s_gaia_type)
        {
            // The T_gaia object will contain the type name we want for the exception.
            T_gaia expected{0};
            throw edc_invalid_object_type(node_ptr->id,
                expected.gaia_type(),
                expected.gaia_typename(),
                it->second->gaia_type(),
                it->second->gaia_typename());
        }
        client_ptr = static_pointer_cast<T_gaia>(it->second);
    }
    else 
    {
        client_ptr.reset(new T_gaia(node_ptr->id));
        s_gaia_cache.insert(pair<gaia_id_t, gaia_base_ptr_t>(node_ptr->id, client_ptr));
    }

    client_ptr->refresh();
    return client_ptr;
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::refresh()
{
    auto node_ptr = gaia_se_node::open(m_id);
    if (node_ptr)
    {
        refresh(node_ptr);
    }
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
void gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::refresh(gaia_ptr<gaia_se_node>& node_ptr)
{
    m_fb = flatbuffers::GetRoot<T_fb>(node_ptr->data());
    m_references = node_ptr->references();
    if (m_writer)
    {
        m_fb->UnPackTo(m_writer.get());
    }
}

template <gaia::db::gaia_type_t T_gaia_type, typename T_gaia, typename T_fb, typename T_obj, size_t N_references>
gaia::db::gaia_type_t gaia_object_t<T_gaia_type, T_gaia, T_fb, T_obj, N_references>::s_gaia_type = T_gaia_type;

} // direct_access
} // gaia
