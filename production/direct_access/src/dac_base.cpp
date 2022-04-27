/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"
#include "gaia/direct_access/dac_base.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/exceptions.hpp"

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

invalid_object_state_internal::invalid_object_state_internal()
{
    m_message = "An operation was attempted on an object that does not exist.";
}

invalid_object_state_internal::invalid_object_state_internal(gaia_id_t parent_id, gaia_id_t child_id, const char* child_type)
{
    stringstream message;
    message
        << "Cannot insert an object of type '" << child_type
        << "' into the container. The parent id '" << parent_id << "' or the child id '"
        << child_id << "' is invalid.";
    m_message = message.str();
}

//
// Implementation of structs derived from dac_base_iterator_state_t
//

struct dac_generator_iterator_state_t : public dac_base_iterator_state_t
{
    ~dac_generator_iterator_state_t() override = default;

    generator_iterator_t<gaia_ptr_t> iterator;
};

//
// dac_db_t implementation
//

std::shared_ptr<dac_base_iterator_state_t> dac_db_t::initialize_iterator(gaia_type_t container_type_id)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    std::shared_ptr<dac_base_iterator_state_t> iterator_state
        = std::make_shared<dac_generator_iterator_state_t>();
    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<dac_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    iterator = gaia_ptr_t::find_all_iterator(container_type_id);
    return iterator_state;
}

gaia_id_t dac_db_t::get_iterator_value(std::shared_ptr<dac_base_iterator_state_t> iterator_state)
{
    ASSERT_PRECONDITION(iterator_state, "Attempt to access unset iterator state!");

    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<dac_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    if (!iterator)
    {
        return c_invalid_gaia_id;
    }
    gaia_ptr_t gaia_ptr = *iterator;
    gaia_id_t gaia_id = gaia_ptr.id();
    ASSERT_INVARIANT(gaia_id.is_valid(), "get_iterator_value() has unexpectedly produced an invalid gaia_id value!");
    return gaia_id;
}

bool dac_db_t::advance_iterator(std::shared_ptr<dac_base_iterator_state_t> iterator_state)
{
    ASSERT_PRECONDITION(iterator_state, "Attempt to advance unset iterator state!");

    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    generator_iterator_t<gaia_ptr_t>& iterator
        = (reinterpret_cast<dac_generator_iterator_state_t*>(iterator_state.get()))->iterator;
    if (!iterator)
    {
        return false;
    }
    return static_cast<bool>(++iterator);
}

// If the object exists, returns true and retrieves the container type of the object.
// Otherwise, returns false.
bool dac_db_t::get_type(gaia_id_t id, gaia_type_t& type)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    gaia_ptr_t gaia_ptr = gaia_ptr_t::from_gaia_id(id);
    if (gaia_ptr)
    {
        type = gaia_ptr.type();
        return true;
    }

    return false;
}

gaia_id_t dac_db_t::get_reference(gaia_id_t id, common::reference_offset_t slot)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    gaia_ptr_t gaia_ptr = gaia_ptr_t::from_gaia_id(id);
    if (!gaia_ptr)
    {
        throw invalid_object_id_internal(id);
    }

    return gaia_ptr.references()[slot];
}

gaia_id_t dac_db_t::insert(gaia_type_t container, size_t data_size, const void* data)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    gaia_id_t id = gaia_ptr_t::generate_id();
    gaia_ptr::create(id, container, data_size, data);
    return id;
}

void dac_db_t::delete_row(gaia_id_t id, bool force)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    gaia_ptr_t gaia_ptr = gaia_ptr_t::from_gaia_id(id);
    if (!gaia_ptr)
    {
        throw invalid_object_id_internal(id);
    }

    gaia_ptr::remove(gaia_ptr, force);
}

void dac_db_t::update(gaia_id_t id, size_t data_size, const void* data)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    gaia_ptr::update_payload(id, data_size, data);
}

bool dac_db_t::insert_into_reference_container(gaia_id_t parent_id, gaia_id_t id, common::reference_offset_t anchor_slot)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    return gaia_ptr::insert_into_reference_container(parent_id, id, anchor_slot);
}

bool dac_db_t::remove_from_reference_container(gaia_id_t parent_id, gaia_id_t id, common::reference_offset_t anchor_slot)
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    return gaia_ptr::remove_from_reference_container(parent_id, id, anchor_slot);
}

//
// dac_base_t implementation
//

static_assert(sizeof(gaia_handle_t) == sizeof(gaia_ptr_t));

void report_invalid_object_id(common::gaia_id_t id)
{
    throw invalid_object_id_internal(id);
}

void report_invalid_object_type(
    common::gaia_id_t id,
    common::gaia_type_t expected_type,
    const char* expected_typename,
    common::gaia_type_t actual_type)
{
    throw invalid_object_type_internal(id, expected_type, expected_typename, actual_type);
}

void report_invalid_object_state()
{
    throw invalid_object_state_internal();
}

void report_invalid_object_state(
    common::gaia_id_t parent_id,
    common::gaia_id_t child_id,
    const char* child_type)
{
    throw invalid_object_state_internal(parent_id, child_id, child_type);
}

template <typename T_ptr>
constexpr T_ptr* dac_base_t::to_ptr()
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    return reinterpret_cast<T_ptr*>(&m_record);
}

template <typename T_ptr>
constexpr const T_ptr* dac_base_t::to_const_ptr() const
{
    if (!is_transaction_open())
    {
        throw no_open_transaction_internal();
    }

    return reinterpret_cast<const T_ptr*>(&m_record);
}

// We only support a single specialization of our ptr functions above using gaia_ptr_t
template gaia_ptr_t* dac_base_t::to_ptr();
template const gaia_ptr_t* dac_base_t::to_const_ptr() const;

dac_base_t::dac_base_t(gaia_id_t id)
{
    *(to_ptr<gaia_ptr_t>()) = gaia_ptr_t::from_gaia_id(id);
}

gaia_id_t dac_base_t::gaia_id() const
{
    auto ptr = to_const_ptr<gaia_ptr_t>();
    if (*ptr)
    {
        return ptr->id();
    }

    return c_invalid_gaia_id;
}

bool dac_base_t::exists() const
{
    return static_cast<bool>(*to_const_ptr<gaia_ptr_t>());
}

const char* dac_base_t::data() const
{
    return to_const_ptr<gaia_ptr_t>()->data();
}

bool dac_base_t::equals(const dac_base_t& other) const
{
    return (*(to_const_ptr<gaia_ptr_t>()) == *(other.to_const_ptr<gaia_ptr_t>()));
}

gaia_id_t* dac_base_t::references() const
{
    return to_const_ptr<gaia_ptr_t>()->references();
}

void dac_base_t::set(common::gaia_id_t new_id)
{
    *(to_ptr<gaia_ptr_t>()) = gaia_ptr_t::from_gaia_id(new_id);
}

//
// dac_base_reference_t implementation
//

dac_base_reference_t::dac_base_reference_t(gaia_id_t parent, reference_offset_t child_offset)
    : m_parent_id(parent), m_child_offset(child_offset)
{
}

bool dac_base_reference_t::connect(gaia_id_t old_id, gaia::common::gaia_id_t new_id)
{
    if (old_id.is_valid() && old_id == new_id)
    {
        return false;
    }
    dac_base_reference_t::disconnect(old_id);
    dac_db_t::insert_into_reference_container(m_parent_id, new_id, m_child_offset);
    return true;
}

bool dac_base_reference_t::disconnect(gaia_id_t id)
{
    if (!id.is_valid())
    {
        return false;
    }
    dac_db_t::remove_from_reference_container(m_parent_id, id, m_child_offset);
    return true;
}

} // namespace direct_access
} // namespace gaia
