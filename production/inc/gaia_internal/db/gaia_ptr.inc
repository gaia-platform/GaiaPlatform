/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

bool gaia_ptr_t::operator==(const gaia_ptr_t& other) const
{
    return m_locator == other.m_locator;
}

bool gaia_ptr_t::operator==(const std::nullptr_t) const
{
    return to_ptr() == nullptr;
}

bool gaia_ptr_t::operator!=(const std::nullptr_t) const
{
    return to_ptr() != nullptr;
}

gaia_ptr_t::operator bool() const
{
    return to_ptr() != nullptr;
}

gaia_ptr_t gaia_ptr_t::operator++()
{
    if (m_locator)
    {
        find_next(to_ptr()->type);
    }
    return *this;
}

gaia_ptr_t gaia_ptr_t::open(
    common::gaia_id_t id)
{
    return gaia_ptr_t(id);
}

bool gaia_ptr_t::is_null() const
{
    return m_locator == 0;
}

gaia_locator_t gaia_ptr_t::locator() const
{
    return m_locator;
}

common::gaia_id_t gaia_ptr_t::id() const
{
    return to_ptr()->id;
}

common::gaia_type_t gaia_ptr_t::type() const
{
    return to_ptr()->type;
}

char* gaia_ptr_t::data() const
{
    return data_size() ? const_cast<char*>(to_ptr()->data()) : nullptr;
}

size_t gaia_ptr_t::data_size() const
{
    size_t total_len = to_ptr()->payload_size;
    size_t refs_len = to_ptr()->num_references * sizeof(common::gaia_id_t);
    size_t data_size = total_len - refs_len;
    return data_size;
}

common::gaia_id_t* gaia_ptr_t::references() const
{
    return const_cast<common::gaia_id_t*>(to_ptr()->references());
}

size_t gaia_ptr_t::num_references() const
{
    return to_ptr()->num_references;
}

bool gaia_ptr_t::is(common::gaia_type_t type) const
{
    return to_ptr() && to_ptr()->type == type;
}
