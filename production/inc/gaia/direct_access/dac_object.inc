/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// NOTE: This is included included by dac_object.hpp as this is a template
// implementation file.  Because the template specializations of dac_object_t are
// created by user-defined schema, we don't know what they will be a priori.

//
// The dac_object_t implementation.
//

// Macros for strongly-typed field accessors used by dac_object_t objects below.
#define GET(field) (row()->field())
#define GET_STR(field) (row()->field() ? row()->field()->c_str() : nullptr)
#define GET_ARRAY(field) (to_dac_vector(row()->field()))

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::dac_object_t(
    gaia::common::gaia_id_t id)
    : dac_base_t(id)
{
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
const T_flatbuffer* dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::row() const
{
    if (!dac_base_t::exists())
    {
        throw gaia::db::invalid_object_id(gaia::common::c_invalid_gaia_id);
    }
    return flatbuffers::GetRoot<T_flatbuffer>(dac_base_t::data());
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::operator bool() const
{
    return dac_base_t::exists();
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
bool dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::operator==(
    const dac_object_t& other) const
{
    return dac_base_t::equals(other);
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
bool dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::operator!=(
    const dac_object_t& other) const
{
    return !dac_base_t::equals(other);
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
gaia::common::gaia_id_t* dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::references() const
{
    if (!dac_base_t::exists())
    {
        throw gaia::db::invalid_object_id(gaia::common::c_invalid_gaia_id);
    }
    return dac_base_t::references();
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
dac_writer_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>
dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::writer()
{
    if (!dac_base_t::exists())
    {
        throw gaia::db::invalid_object_id(gaia::common::c_invalid_gaia_id);
    }

    auto writer = dac_writer_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>();
    writer.m_gaia.id = gaia_id();
    row()->UnPackTo(&writer);
    return writer;
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
T_class dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::get(gaia::common::gaia_id_t id)
{
    return T_class(verify_type(id));
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
gaia::common::gaia_id_t dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::verify_type(gaia::common::gaia_id_t id)
{
    gaia::common::gaia_type_t target_type = gaia::common::c_invalid_gaia_type;
    if (!dac_base_t::get_type(id, target_type))
    {
        return gaia::common::c_invalid_gaia_id;
    }

    // Make sure the types match.
    if (target_type != T_class::s_gaia_type)
    {
        throw invalid_object_type(id, T_class::s_gaia_type, T_class::gaia_typename(), target_type);
    }

    return id;
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
gaia::common::gaia_id_t dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::insert_row(
    flatbuffers::FlatBufferBuilder& fbb)
{
    return dac_base_t::insert(container_type_id, fbb.GetSize(), fbb.GetBufferPointer());
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
void dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::delete_row()
{
    if (!dac_base_t::exists())
    {
        throw gaia::db::invalid_object_id(gaia::common::c_invalid_gaia_id);
    }

    delete_row(gaia_id());
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
void dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::delete_row(gaia::common::gaia_id_t id)
{
    dac_base_t::delete_row(verify_type(id));
}
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_gaia, typename T_fb, typename T_obj>
dac_object_t<container_type_id, T_gaia, T_fb, T_obj>::dac_object_t()
    : dac_base_t()
{
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
gaia::common::gaia_type_t dac_object_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::s_gaia_type
    = container_type_id;

//
// The dac_writer_t implementation.
//
template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
gaia::common::gaia_id_t dac_writer_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::insert_row()
{
    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    gaia::common::gaia_id_t id = dac_db_t::insert(container_type_id, m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();
    return id;
}

template <gaia::common::gaia_type_t::value_type container_type_id, typename T_class, typename T_flatbuffer, typename T_flatbuffer_object>
void dac_writer_t<container_type_id, T_class, T_flatbuffer, T_flatbuffer_object>::update_row()
{
    auto u = T_flatbuffer::Pack(m_builder, this);
    m_builder.Finish(u);
    dac_db_t::update(m_gaia.id, m_builder.GetSize(), m_builder.GetBufferPointer());
    m_builder.Clear();
}
