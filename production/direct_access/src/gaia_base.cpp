/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_base.hpp"

#include "gaia_db.hpp"
#include "gaia_ptr.hpp"

using namespace gaia::db;

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

gaia_id_t gaia_base_t::insert(gaia_type_t container, size_t num_refs, size_t data_size, const void* data)
{
    gaia_id_t node_id = gaia_ptr::generate_id();
    gaia_ptr::create(node_id, container, num_refs, data_size, data);
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

// Returns true if the child was already inserted; false otherwise.
bool gaia_base_t::insert_reference(
    gaia_id_t parent_id, gaia_id_t child_id, size_t parent_slot, size_t child_slot, size_t next_child_slot)
{
    bool already_inserted = true;
    gaia_ptr parent = gaia_ptr::open(parent_id);
    if (!parent)
    {
        return false;
    }

    gaia_ptr child = gaia_ptr::open(child_id);
    if (!child)
    {
        return false;
    }

    if (child.references()[parent_slot] == parent_id)
    {
        return false;
    }

    if (child.references()[parent_slot] != 0 && child.references()[parent_slot] != parent_id)
    {
        return already_inserted;
    }

    child.update_child_references(next_child_slot, parent.references()[child_slot], parent_slot, parent_id);
    parent.update_child_reference(child_slot, child_id);
    return false;
}

// Returns true if an error occurred.  If the error was due to an invalid_member then
// the invalid_member bool is set to true.  Otherwise, the error is due to an inconsistent list.
bool gaia_base_t::remove_reference(
    gaia_id_t parent_id,
    gaia_id_t child_id,
    size_t parent_slot,
    size_t child_slot,
    size_t next_child_slot,
    bool& invalid_member)
{
    bool error_occurred = true;

    gaia_ptr child = gaia_ptr::open(child_id);
    if (!child)
    {
        return false;
    }

    if (child.references()[parent_slot] != parent_id)
    {
        invalid_member = true;
        return error_occurred;
    }

    gaia_ptr parent = gaia_ptr::open(parent_id);
    if (!parent)
    {
        return false;
    }

    if (parent.references()[child_slot] == child_id)
    {
        // It's the first one in the list, point the "first" to the current "next".
        parent.update_child_reference(child_slot, child.references()[next_child_slot]);
        // Clean up the removed child.
        child.update_child_references(next_child_slot, c_invalid_gaia_id, parent_slot, c_invalid_gaia_id);
    }
    else
    {
        // Need to scan the list to find this one because it's not first on the list.
        gaia_ptr cur_child_ptr = gaia_ptr::open(parent.references()[child_slot]);

        while (cur_child_ptr && cur_child_ptr.references()[next_child_slot])
        {
            gaia_id_t next_id = cur_child_ptr.references()[next_child_slot];
            if (next_id == child_id)
            {
                // Point the current child to the child following the next.
                cur_child_ptr.update_child_reference(next_child_slot, child.references()[next_child_slot]);
                // Clean up the removed child.
                child.update_child_references(next_child_slot, c_invalid_gaia_id, parent_slot, c_invalid_gaia_id);
                return false;
            }
            // Move to the next child.
            cur_child_ptr = gaia_ptr::open(next_id);
        }
        // If we end up here, the child was not found in the chain. This is an error because
        // the pointers have become inconsistent (the child's parent pointer was correct).
        return error_occurred;
    }

    return false;
}

} // namespace direct_access
} // namespace gaia
