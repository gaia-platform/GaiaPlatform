/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/direct_access/edc_base.hpp"

#include "gaia/db/db.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::db;
using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace direct_access
{

//
// Exception class implementations.
//
edc_invalid_object_type::edc_invalid_object_type(gaia_id_t id, gaia_type_t expected_type, const char* expected_typename, gaia_type_t actual_type)
{
    stringstream msg;
    msg << "Requesting Gaia type " << expected_typename << "(" << expected_type << ") but object identified by " << id
        << " is type (" << actual_type << ").";
    m_message = msg.str();
}

edc_invalid_member::edc_invalid_member(gaia_id_t id, gaia_type_t parent, const char* parent_type, gaia_type_t child, const char* child_name)
{
    stringstream msg;
    msg << "Attempting to remove record with Gaia type " << child_name << "(" << child << ") from parent " << id
        << " of type " << parent_type << "(" << parent << "), but record is not a member.";
    m_message = msg.str();
}

edc_inconsistent_list::edc_inconsistent_list(gaia_id_t id, const char* parent_type, gaia_id_t child, const char* child_name)
{
    stringstream msg;
    msg << "List is inconsistent, child points to parent " << id << " of type " << parent_type << ", but child ("
        << child << ", type " << child_name << ") is not in parent's list.";
    m_message = msg.str();
}

edc_invalid_state::edc_invalid_state(const char* parent_type, const char* child_type)
{
    stringstream msg;
    msg << "Object(s) have invalid state, no ID exists. Parent type is " << parent_type << " and child type is "
        << child_type << ".";
    m_message = msg.str();
}

edc_already_inserted::edc_already_inserted(gaia_id_t parent, const char* parent_type)
{
    stringstream msg;
    msg << "The object being inserted is a member of this same list type but a different owner. "
           "The owner object type is "
        << parent_type << ", and id is " << parent << ".";
    m_message = msg.str();
}

static_assert(sizeof(gaia_handle_t) == sizeof(gaia_ptr));

template <typename T_ptr>
constexpr T_ptr* gaia_base_t::to_ptr()
{
    return reinterpret_cast<T_ptr*>(&m_record);
}

template <typename T_ptr>
constexpr const T_ptr* gaia_base_t::to_const_ptr() const
{
    return reinterpret_cast<const T_ptr*>(&m_record);
}

// We only support a single specialization of our ptr functions above using gaia_ptr
template gaia_ptr* gaia_base_t::to_ptr();
template const gaia_ptr* gaia_base_t::to_const_ptr() const;

gaia_base_t::gaia_base_t(const char* gaia_typename)
    : m_typename(gaia_typename)
{
    *(to_ptr<gaia_ptr>()) = gaia_ptr();
}

gaia_base_t::gaia_base_t(const char* gaia_typename, gaia_id_t id)
    : m_typename(gaia_typename)
{
    *(to_ptr<gaia_ptr>()) = gaia_ptr(id);
}

gaia_id_t gaia_base_t::id() const
{
    auto ptr = to_const_ptr<gaia_ptr>();
    if (*ptr)
    {
        return ptr->id();
    }

    return c_invalid_gaia_id;
}

bool gaia_base_t::exists() const
{
    return static_cast<bool>(*to_const_ptr<gaia_ptr>());
}

// Returns whether a node with the given id exists in the
// database. If so, it fills in the container type of the
// node.
bool gaia_base_t::exists(gaia_id_t id, gaia_type_t& type)
{
    gaia_ptr node_ptr = gaia_ptr::open(id);
    if (node_ptr)
    {
        type = node_ptr.type();
        return true;
    }

    return false;
}

const char* gaia_base_t::data() const
{
    return to_const_ptr<gaia_ptr>()->data();
}

bool gaia_base_t::equals(const gaia_base_t& other) const
{
    return (*(to_const_ptr<gaia_ptr>()) == *(other.to_const_ptr<gaia_ptr>()));
}

gaia_id_t* gaia_base_t::references()
{
    return to_ptr<gaia_ptr>()->references();
}

gaia_id_t gaia_base_t::get_reference(gaia_id_t id, size_t slot)
{
    gaia_ptr node_ptr = gaia_ptr::open(id);
    return node_ptr.references()[slot];
}

gaia_id_t gaia_base_t::find_first(gaia_type_t container)
{
    gaia_ptr node = gaia_ptr::find_first(container);
    if (!node)
    {
        return c_invalid_gaia_id;
    }

    return node.id();
}

gaia_id_t gaia_base_t::find_next()
{
    gaia_ptr node = to_ptr<gaia_ptr>()->find_next();
    if (node)
    {
        return node.id();
    }
    return c_invalid_gaia_id;
}

gaia_id_t gaia_base_t::find_next(gaia_id_t id)
{
    auto node_ptr = gaia_ptr(id);
    gaia_id_t next_id = c_invalid_gaia_id;
    if (node_ptr)
    {
        node_ptr = node_ptr.find_next();
        if (node_ptr)
        {
            next_id = node_ptr.id();
        }
    }
    return next_id;
}

gaia_id_t gaia_base_t::insert(gaia_type_t container, size_t data_size, const void* data)
{
    gaia_id_t node_id = gaia_ptr::generate_id();
    gaia_ptr::create(node_id, container, data_size, data);
    return node_id;
}

void gaia_base_t::delete_row(gaia_id_t id)
{
    gaia_ptr node_ptr = gaia_ptr::open(id);
    if (!node_ptr)
    {
        throw invalid_node_id(id);
    }

    gaia_ptr::remove(node_ptr);
}

void gaia_base_t::update(gaia_id_t id, size_t data_size, const void* data)
{
    gaia_ptr node_ptr = gaia_ptr::open(id);
    if (!node_ptr)
    {
        throw invalid_node_id(id);
    }
    node_ptr.update_payload(data_size, data);
}

void gaia_base_t::insert_child_reference(gaia_id_t parent_id, gaia_id_t child_id, size_t child_slot)
{
    gaia_ptr parent = gaia_ptr::open(parent_id);
    if (!parent)
    {
        throw invalid_node_id(parent_id);
    }

    parent.add_child_reference(child_id, child_slot);
}

void gaia_base_t::remove_child_reference(gaia_id_t parent_id, gaia_id_t child_id, size_t child_slot)
{
    gaia_ptr parent = gaia_ptr::open(parent_id);
    if (!parent)
    {
        throw invalid_node_id(parent_id);
    }

    parent.remove_child_reference(child_id, child_slot);
}

} // namespace direct_access
} // namespace gaia
