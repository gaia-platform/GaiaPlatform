/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// NOTE: This is included included by edc_object.hpp as this is a template
// implementation file.  Because the template specializations of gaia_object_t are
// created by user-defined schema, we don't know what they will be a priori.
namespace gaia
{
namespace direct_access
{

//
// The gaia_object_t implementation.
//

// Macros for strongly-typed field accessors used by gaia_object_t objects below.
#define GET(field) (row()->field())
#define GET_STR(field) (row()->field() ? row()->field()->c_str() : nullptr)

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_object_t(
    const char* gaia_typename)
    : gaia_base_t(gaia_typename)
{
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_object_t(
    gaia::common::gaia_id_t id, const char* gaia_typename)
    : gaia_base_t(gaia_typename, id)
{
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_id_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::gaia_id() const
{
    return gaia_base_t::id();
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
const T_flatbuffer* gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::row() const
{
    if (!gaia_base_t::exists())
    {
        throw gaia::db::invalid_node_id(0);
    }
    return flatbuffers::GetRoot<T_flatbuffer>(gaia_base_t::data());
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::operator bool() const
{
    return gaia_base_t::exists();
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
bool gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::operator==(
    const gaia_object_t& other) const
{
    return gaia_base_t::equals(other);
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_id_t* gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::references()
{
    if (!gaia_base_t::exists())
    {
        throw gaia::db::invalid_node_id(0);
    }
    return gaia_base_t::references();
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>
gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::writer()
{
    if (!gaia_base_t::exists())
    {
        throw gaia::db::invalid_node_id(0);
    }

    auto writer = gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>();
    writer.m_gaia.id = id();
    row()->UnPackTo(&writer);
    return writer;
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get_first()
{
    gaia::common::gaia_id_t id = gaia_base_t::find_first(T_container);
    return T_class(id);
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get_next()
{
    if (!gaia_base_t::exists())
    {
        throw gaia::db::invalid_node_id(0);
    }
    gaia::common::gaia_id_t id = gaia_base_t::find_next();
    return T_class(id);
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
T_class gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::get(gaia::common::gaia_id_t id)
{
    return T_class(verify_type(id));
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_id_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::verify_type(gaia::common::gaia_id_t id)
{
    gaia::common::gaia_type_t target_type = gaia::common::c_invalid_gaia_type;
    if (!gaia_base_t::get_type(id, target_type))
    {
        return gaia::common::c_invalid_gaia_id;
    }

    // Make sure the types match.
    if (target_type != T_class::s_gaia_type)
    {
        T_class expected = T_class::get(0);
        throw edc_invalid_object_type(id, T_class::s_gaia_type, expected.gaia_typename(), target_type);
    }

    return id;
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_id_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::insert_row(
    flatbuffers::FlatBufferBuilder& fbb)
{
    return gaia_base_t::insert(T_container, fbb.GetSize(), fbb.GetBufferPointer());
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
void gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::delete_row()
{
    if (!gaia_base_t::exists())
    {
        throw gaia::db::invalid_node_id(0);
    }

    delete_row(gaia_id());
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
void gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::delete_row(gaia::common::gaia_id_t id)
{
    gaia_base_t::delete_row(verify_type(id));
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_type_t gaia_object_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::s_gaia_type
    = T_container;

//
// The gaia_writer_t implementation.
//
template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
gaia::common::gaia_id_t gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::insert_row()
{
    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    gaia::common::gaia_id_t id = edc_db_t::insert(T_container, m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();
    return id;
}

template <gaia::common::gaia_type_t T_container, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object, size_t N_references>
void gaia_writer_t<T_container, T_class, T_flatbuffer, T_flatbuffer_object, N_references>::update_row()
{
    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    edc_db_t::update(m_gaia.id, m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();
}

} // namespace direct_access
} // namespace gaia
