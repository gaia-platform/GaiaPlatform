/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <record_list.hpp>

#include <retail_assert.hpp>
#include <locator_allocator.hpp>

using namespace gaia::common;
using namespace gaia::db::storage;

record_data_t::record_data_t()
{
    locator = c_invalid_locator;
    is_deleted = false;
}

void record_data_t::set(uint64_t locator)
{
    retail_assert(locator != c_invalid_locator, "An invalid locator was passed to record_data_t::set()!");

    this->locator = locator;
    this->is_deleted = false;
}

record_range_t::record_range_t(size_t range_size)
{
    retail_assert(range_size > 0, "Range size must be greater than 0");

    m_range_size = range_size;
    m_record_range = new record_data_t[m_range_size];
    m_next_available_index = 0;
    m_has_deletions = false;
    m_next_range = nullptr;
}

record_range_t::~record_range_t()
{
    delete [] m_record_range;
}

bool record_range_t::is_full()
{
    return m_next_available_index == m_range_size;
}

void record_range_t::compact()
{
    if (!m_has_deletions)
    {
        return;
    }

    // Compact current range.
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < m_next_available_index; read_index++)
    {
        if (m_record_range[read_index].is_deleted == false)
        {
            m_record_range[write_index++] = m_record_range[read_index];
        }
    }

    m_next_available_index = write_index;
    m_has_deletions = false;
}

void record_range_t::add(uint64_t locator)
{
    retail_assert(locator != c_invalid_locator, "An invalid locator was passed to record_range_t::add()!");
    retail_assert(!is_full(), "Range is full!");

    int index = m_next_available_index++;

    m_record_range[index].set(locator);
}

record_data_t& record_range_t::get(size_t index)
{
    retail_assert(index < m_range_size, "Range index is out of bounds!");
    retail_assert(index < m_next_available_index, "Range index is out of allocated bounds!");

    return m_record_range[index];
}

void record_range_t::add_next_range()
{
    retail_assert(m_next_range == nullptr, "Cannot add next range because one already exists!");

    m_next_range = new record_range_t(m_range_size);
}

record_range_t* record_range_t::next_range()
{
    return m_next_range;
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

record_list_t::record_list_t(size_t range_size)
{
    retail_assert(range_size > 0, "Range size must be greater than 0");

    m_range_size = range_size;
    m_record_ranges = new record_range_t(m_range_size);
}

record_list_t::~record_list_t()
{
    // Iterate over the range list and deallocate each range.
    record_range_t* current_range = m_record_ranges;
    while (current_range != nullptr)
    {
        record_range_t* next_range = current_range->next_range();
        delete current_range;
        current_range = next_range;
    }
}

void record_list_t::compact()
{
    // Iterate over our list and compact each range.
    for (record_range_t* current_range = m_record_ranges;
        current_range != nullptr;
        current_range = current_range->next_range())
    {
        current_range->compact();
    }
}

void record_list_t::add(uint64_t locator)
{
    retail_assert(locator != c_invalid_locator, "An invalid locator was passed to record_list_t::add()!");

    // We'll iterate over our list and look for a range with available space.
    record_range_t* current_range = m_record_ranges;
    while (current_range != nullptr)
    {
        // If current range has space, add our record to it.
        if (!current_range->is_full())
        {
            current_range->add(locator);
            return;
        }

        // If we reached the last range, add a new one.
        if (current_range->next_range() == nullptr)
        {
            current_range->add_next_range();
        }

        // Check the next range.
        // This is guaranteed to exist because of the above logic.
        current_range = current_range->next_range();
    }
}

// Seek for the first valid record starting from the current iterator position.
void record_list_t::seek(record_iterator_t& iterator)
{
    while (!iterator.at_end())
    {
        for ( ; iterator.current_index < iterator.current_range->m_next_available_index; iterator.current_index++)
        {
            if (iterator.current_range->get(iterator.current_index).is_deleted == false)
            {
                return;
            }
        }

        iterator.current_range = iterator.current_range->next_range();
        iterator.current_index = 0;
    }
}

bool record_list_t::start(record_iterator_t& iterator)
{
    // Position iterator to beginning of list.
    iterator.current_range = m_record_ranges;
    iterator.current_index = 0;

    // Seek first valid record.
    seek(iterator);

    return (iterator.at_end() == false);
}

bool record_list_t::move_next(record_iterator_t& iterator)
{
    // If already at end, just point that out.
    if (iterator.at_end())
    {
        return false;
    }

    // Increment current index - this is enough to make seek work properly.
    iterator.current_index++;

    // See first valid record.
    seek(iterator);

    return (iterator.at_end() == false);
}

uint64_t record_list_t::get_record_locator(record_iterator_t& iterator)
{
    retail_assert(iterator.at_end() == false, "Attempt to access invalid iterator state!");

    return iterator.current_range->get(iterator.current_index).locator;
}

void  record_list_t::delete_record(record_iterator_t& iterator)
{
    retail_assert(iterator.at_end() == false, "Attempt to access invalid iterator state!");

    iterator.current_range->get(iterator.current_index).is_deleted = true;
}
