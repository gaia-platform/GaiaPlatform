/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/direct_access/edc_base.hpp"

#include "gaia/db/db.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

using namespace gaia::db;
using namespace std;
using namespace gaia::common;
using namespace gaia::common::iterators;

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
    msg << "Requesting Gaia type '" << expected_typename << "'('" << expected_type
        << "'), but object identified by '" << id << "' is of type '" << actual_type << "'.";
    m_message = msg.str();
}

edc_invalid_member::edc_invalid_member(gaia_id_t id, gaia_type_t parent, const char* parent_type, gaia_type_t child, const char* child_name)
{
    stringstream msg;
    msg << "Attempting to remove record with Gaia type '" << child_name << "'('" << child << "') from parent '" << id
        << "' of type '" << parent_type << "'('" << parent << "'), but the record is not a member of such a relationship.";
    m_message = msg.str();
}

edc_inconsistent_list::edc_inconsistent_list(gaia_id_t id, const char* parent_type, gaia_id_t child, const char* child_name)
{
    stringstream msg;
    msg << "List is inconsistent; child points to parent '" << id << "' of type '" << parent_type << "', but child '"
        << child << "' of type '" << child_name << "' is not in parent's list.";
    m_message = msg.str();
}

edc_invalid_state::edc_invalid_state(gaia_id_t parent_id, gaia_id_t child_id, const char* child_type)
{
    stringstream msg;
    msg << "Cannot insert an object of type '" << child_type
        << "' into the container. The parent id '" << parent_id << "' or the child id '"
        << child_id << "' is invalid.";
    m_message = msg.str();
}

edc_already_inserted::edc_already_inserted(gaia_id_t parent, const char* parent_type)
{
    stringstream msg;
    msg << "The object being inserted is a member of this same list type but has a different owner. "
        << "The owner object type is '" << parent_type << "', and its ID is '" << parent << "'.";
    m_message = msg.str();
}

//
// Implementation of structs derived from edc_base_iterator_state_t
//

struct edc_generator_iterator_state_t : public edc_base_iterator_state_t
{
    ~edc_generator_iterator_state_t() override = default;

    generator_iterator_t<gaia_ptr_t> iterator;
};

//
// edc_db_t implementation
//

std::shared_ptr<edc_base_iterator_state_t> edc_db_t::initialize_iterator(gaia_type_t container_type_id)
{
    std::shared_ptr<edc_base_iterator_state_t> iterator_state;
    iterator_state.reset(new edc_generator_iterator_state_t());
    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<edc_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    iterator = gaia_ptr_t::find_all_iterator(container_type_id);
    return iterator_state;
}

gaia_id_t edc_db_t::get_iterator_value(std::shared_ptr<edc_base_iterator_state_t> iterator_state)
{
    ASSERT_PRECONDITION(iterator_state, "Attempt to access unset iterator state!");

    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<edc_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    if (!iterator)
    {
        return c_invalid_gaia_id;
    }
    gaia_ptr_t gaia_ptr = *iterator;
    return gaia_ptr.id();
}

bool edc_db_t::advance_iterator(std::shared_ptr<edc_base_iterator_state_t> iterator_state)
{
    ASSERT_PRECONDITION(iterator_state, "Attempt to advance unset iterator state!");

    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<edc_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    if (!iterator)
    {
        return false;
    }
    return static_cast<bool>(++iterator);
}

// If the object exists, returns true and retrieves the container type of the object.
// Otherwise, returns false.
bool edc_db_t::get_type(gaia_id_t id, gaia_type_t& type)
{
    gaia_ptr_t gaia_ptr = gaia_ptr_t::open(id);
    if (gaia_ptr)
    {
        type = gaia_ptr.type();
        return true;
    }

    return false;
}

gaia_id_t edc_db_t::get_reference(gaia_id_t id, size_t slot)
{
    gaia_ptr_t gaia_ptr = gaia_ptr_t::open(id);
    return gaia_ptr.references()[slot];
}

gaia_id_t edc_db_t::insert(gaia_type_t container, size_t data_size, const void* data)
{
    gaia_id_t id = gaia_ptr_t::generate_id();
    gaia_ptr_t::create(id, container, data_size, data);
    return id;
}

void edc_db_t::delete_row(gaia_id_t id)
{
    gaia_ptr_t gaia_ptr = gaia_ptr_t::open(id);
    if (!gaia_ptr)
    {
        throw invalid_object_id(id);
    }

    gaia_ptr_t::remove(gaia_ptr);
}

void edc_db_t::update(gaia_id_t id, size_t data_size, const void* data)
{
    gaia_ptr_t gaia_ptr = gaia_ptr_t::open(id);
    if (!gaia_ptr)
    {
        throw invalid_object_id(id);
    }
    gaia_ptr.update_payload(data_size, data);
}

void edc_db_t::insert_child_reference(gaia_id_t parent_id, gaia_id_t child_id, size_t child_slot)
{
    gaia_ptr_t parent = gaia_ptr_t::open(parent_id);
    if (!parent)
    {
        throw invalid_object_id(parent_id);
    }

    parent.add_child_reference(child_id, child_slot);
}

void edc_db_t::remove_child_reference(gaia_id_t parent_id, gaia_id_t child_id, size_t child_slot)
{
    gaia_ptr_t parent = gaia_ptr_t::open(parent_id);
    if (!parent)
    {
        throw invalid_object_id(parent_id);
    }

    parent.remove_child_reference(child_id, child_slot);
}

//
// edc_base_t implementation
//

static_assert(sizeof(gaia_handle_t) == sizeof(gaia_ptr_t));

template <typename T_ptr>
constexpr T_ptr* edc_base_t::to_ptr()
{
    return reinterpret_cast<T_ptr*>(&m_record);
}

template <typename T_ptr>
constexpr const T_ptr* edc_base_t::to_const_ptr() const
{
    return reinterpret_cast<const T_ptr*>(&m_record);
}

// We only support a single specialization of our ptr functions above using gaia_ptr_t
template gaia_ptr_t* edc_base_t::to_ptr();
template const gaia_ptr_t* edc_base_t::to_const_ptr() const;

edc_base_t::edc_base_t(const char* gaia_typename)
    : m_typename(gaia_typename)
{
    *(to_ptr<gaia_ptr_t>()) = gaia_ptr_t();
}

edc_base_t::edc_base_t(const char* gaia_typename, gaia_id_t id)
    : m_typename(gaia_typename)
{
    *(to_ptr<gaia_ptr_t>()) = gaia_ptr_t(id);
}

gaia_id_t edc_base_t::gaia_id() const
{
    auto ptr = to_const_ptr<gaia_ptr_t>();
    if (*ptr)
    {
        return ptr->id();
    }

    return c_invalid_gaia_id;
}

bool edc_base_t::exists() const
{
    return static_cast<bool>(*to_const_ptr<gaia_ptr_t>());
}

const char* edc_base_t::data() const
{
    return to_const_ptr<gaia_ptr_t>()->data();
}

bool edc_base_t::equals(const edc_base_t& other) const
{
    return (*(to_const_ptr<gaia_ptr_t>()) == *(other.to_const_ptr<gaia_ptr_t>()));
}

gaia_id_t* edc_base_t::references() const
{
    return to_const_ptr<gaia_ptr_t>()->references();
}

//
// edc_ref_t implementation
//
edc_ref_t::edc_ref_t(gaia::common::gaia_id_t parent)
    : m_parent_id(parent)
{
}

void edc_ref_t::connect(gaia::common::gaia_id_t id)
{
    edc_db_t::insert_child_reference(m_parent_id, id, 0);
}

void edc_ref_t::disconnect(gaia::common::gaia_id_t id)
{
    edc_db_t::remove_child_reference(m_parent_id, id, 0);
}

} // namespace direct_access
} // namespace gaia
