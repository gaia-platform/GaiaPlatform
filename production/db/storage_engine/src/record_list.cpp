/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <record_list.hpp>

#include <retail_assert.hpp>
#include <locator_allocator.hpp>

using namespace gaia::common;
using namespace gaia::db::storage;

const uint64_t record_range_t::c_range_size = 1024;

record_data_t::record_data_t()
{
    locator = c_invalid_locator;
}

record_range_t::record_range_t()
{
    m_record_range = new record_data_t[c_range_size];
    m_next_available_index = 0;
    m_has_deletions = false;
    m_next_range = nullptr;

}

record_range_t::~record_range_t()
{
    delete [] m_record_range;
}

void record_range_t::compact()
{
    if (!m_has_deletions)
    {
        return;
    }

}

record_data_t record_range_t::get(size_t index)
{
    retail_assert(index < c_range_size, "Range index is out of bounds!");

    return m_record_range[index];
}


record_iterator_t::record_iterator_t()
{
    current_range = nullptr;
    current_index = 0;
}

bool record_iterator_t::at_end()
{
    return current_range == nullptr;
}

record_list_t::record_list_t()
{
    m_record_ranges = new record_range_t();
}

record_list_t::~record_list_t()
{
    // TODO: Deallocate entire list.
}

void record_list_t::add(uint64_t locator)
{
    retail_assert(locator != c_invalid_locator, "An invalid locator was passed to record_list_t::add()!");
}

record_iterator_t record_list_t::start()
{
    record_iterator_t iterator;

    return iterator;
}

bool record_list_t::move_next(record_iterator_t& iterator)
{
    if (iterator.at_end())
    {
        return false;
    }

    return false;
}

uint64_t record_list_t::get_record_locator(record_iterator_t& iterator)
{
    retail_assert(iterator.at_end() == false, "Attempt to access invalid iterator state!");

    return iterator.current_range->get(iterator.current_index).locator;
}
