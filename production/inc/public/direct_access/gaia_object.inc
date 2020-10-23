/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

using namespace gaia::db;

// NOTE: This is included included by gaia_object.hpp as this is a template
// implementation file.  Because the template specializations of gaia_object_t are
// created by user-defined schema, we don't know what they will be apriori.  I have
// pulled them out of gaia_object.hpp, however, to include readability of the
// gaia_object_t declarations.
namespace gaia
{
namespace direct_access
{

//
// The gaia_object_t implementation.
//

// Macros for strongly types field accessors used by gaia_object_t objects below.
#define GET(field) (row()->field())
#define GET_STR(field) (row()->field() ? row()->field()->c_str() : nullptr)

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_object_t(
    const char* gaia_typename)
    : gaia_base_t(gaia_typename)
{
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_object_t(
    gaia_id_t id, const char* gaia_typename)
    : gaia_base_t(gaia_typename)
{
    m_record = gaia_ptr::open(id);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_id_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_id() const
{
    return m_record ? m_record.id() : INVALID_GAIA_ID;
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
const T_flatbuffer* gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::row() const
{
    if (!m_record)
    {
        throw invalid_node_id(0);
    }
    return flatbuffers::GetRoot<T_flatbuffer>(m_record.data());
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::operator bool() const
{
    return static_cast<bool>(m_record);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
bool gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::operator==(
    const gaia_object_t& other) const
{
    return m_record == other.m_record;
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_id_t* gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::references()
{
    if (!m_record)
    {
        throw invalid_node_id(0);
    }
    return m_record.references();
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::writer()
{
    if (!m_record)
    {
        throw invalid_node_id(0);
    }

    auto writer = gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>();
    writer.m_gaia.id = m_record.id();
    row()->UnPackTo(&writer);
    return writer;
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get_first()
{
    auto node_ptr = gaia_ptr::find_first(T_container);
    return get_object(node_ptr);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get_next()
{
    if (!m_record)
    {
        throw invalid_node_id(0);
    }

    auto next_ptr = m_record.find_next();
    return get_object(next_ptr);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get(gaia_id_t id)
{
    auto node_ptr = gaia_ptr::open(id);
    return get_object(node_ptr);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_id_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::insert_row(
    flatbuffers::FlatBufferBuilder& fbb)
{
    gaia_id_t node_id = gaia_ptr::generate_id();
    gaia_ptr::create(node_id, T_container, N_references, fbb.GetSize(), fbb.GetBufferPointer());
    return node_id;
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
void gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::delete_row()
{
    if (!m_record)
    {
        throw invalid_node_id(0);
    }

    delete_row(gaia_id());
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
void gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::delete_row(gaia_id_t id)
{
    auto node_ptr = gaia_ptr::open(id);
    if (!node_ptr)
    {
        throw invalid_node_id(id);
    }

    gaia_ptr::remove(node_ptr);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
T_class
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get_object(gaia_ptr& node_ptr)
{
    gaia_id_t id = 0;
    if (node_ptr)
    {
        // Make sure the types match
        if (node_ptr.type() != T_class::s_gaia_type)
        {
            T_class expected = T_class::get(0);
            throw edc_invalid_object_type(node_ptr.id(), T_class::s_gaia_type, expected.gaia_typename(),
                                          node_ptr.type());
        }
        id = node_ptr.id();
    }
    return T_class(id);
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_type_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::s_gaia_type
    = T_container;

//
// The gaia_writer_t implementation.
//
template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
gaia_id_t gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::insert_row()
{
    gaia_ptr node_ptr;

    gaia_id_t id = gaia_ptr::generate_id();
    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    node_ptr = gaia_ptr::create(id, T_container, N_references, m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();

    return id;
}

template <gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object,
          size_t N_references>
void gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::update_row()
{
    auto node_ptr = gaia_ptr::open(m_gaia.id);
    if (!node_ptr)
    {
        throw invalid_node_id(m_gaia.id);
    }

    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    node_ptr.update_payload(m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();
}

} // namespace direct_access
} // namespace gaia
