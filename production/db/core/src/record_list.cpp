/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "record_list.hpp"

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::storage;

record_data_t::record_data_t()
{
    locator = c_invalid_gaia_locator;
}

record_range_t::record_range_t(size_t range_size)
{
    ASSERT_PRECONDITION(range_size > 0, "Range size must be greater than 0");

    m_range_size = range_size;
    m_record_range = new record_data_t[m_range_size];
    m_next_available_index = 0;
    m_has_deletions = false;
    m_next_range = nullptr;
}

record_range_t::~record_range_t()
{
    delete[] m_record_range;
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
        if (m_record_range[read_index].locator != c_invalid_gaia_locator)
        {
            m_record_range[write_index++] = m_record_range[read_index];
        }
    }

    m_next_available_index = write_index;
    m_has_deletions = false;
}

void record_range_t::add(gaia_locator_t locator)
{
    ASSERT_PRECONDITION(locator != c_invalid_gaia_locator, "An invalid locator was passed to record_range_t::add()!");
    ASSERT_PRECONDITION(!is_full(), "Range is full!");

    int index = m_next_available_index++;

    m_record_range[index].locator = locator;
}

record_data_t& record_range_t::get(size_t index)
{
    ASSERT_PRECONDITION(m_record_range != nullptr, "Range is not allocated!");
    ASSERT_PRECONDITION(index < m_range_size, "Range index is out of bounds!");
    ASSERT_PRECONDITION(index < m_next_available_index, "Range index is out of allocated bounds!");

    return m_record_range[index];
}

void record_range_t::add_next_range()
{
    ASSERT_PRECONDITION(m_next_range == nullptr, "Cannot add next range because one already exists!");

    m_next_range = new record_range_t(m_range_size);
}

record_iterator_t::record_iterator_t()
{
    current_range = nullptr;
    current_index = 0;
}

record_iterator_t::~record_iterator_t()
{
    // Release the shared lock maintained on the current range.
    if (current_range != nullptr)
    {
        current_range->m_lock.unlock_shared();
    }
}

record_list_t::record_list_t(size_t range_size)
{
    ASSERT_PRECONDITION(range_size > 0, "Range size must be greater than 0");

    m_range_size = range_size;
    m_record_ranges = new record_range_t(m_range_size);

    m_approximate_count_deletions = 0;
}

record_list_t::~record_list_t()
{
    clear();
}

void record_list_t::clear()
{
    // Iterate over the range list and deallocate each range.
    while (m_record_ranges != nullptr)
    {
        record_range_t* current_range = m_record_ranges;
        m_record_ranges = current_range->next_range();
        delete current_range;
    }

    ASSERT_POSTCONDITION(m_record_ranges == nullptr, "Failed to clear record_list!");
}

void record_list_t::reset()
{
    // Clear record list.
    clear();

    // Create a fresh new range.
    m_record_ranges = new record_range_t(m_range_size);
}

void record_list_t::compact()
{
    // Iterate over our list and look for ranges that can be compacted.
    for (record_range_t* current_range = m_record_ranges;
         current_range != nullptr;
         current_range = current_range->next_range())
    {
        if (current_range->has_deletions())
        {
            // Take an exclusive lock before compacting the range.
            unique_lock unique_range_lock(current_range->m_lock);

            current_range->compact();
        }
    }
}

void record_list_t::add(gaia_locator_t locator)
{
    ASSERT_PRECONDITION(locator != c_invalid_gaia_locator, "An invalid locator was passed to record_list_t::add()!");

    // We'll iterate over our list and look for a range with available space.
    record_range_t* current_range = m_record_ranges;
    while (current_range != nullptr)
    {
        // Only take a lock on the current range if it still has space available
        // or if it's the last range and we need to add a new one.
        if (!current_range->is_full() || current_range->next_range() == nullptr)
        {
            // Take an exclusive lock before attempting to add the new locator.
            unique_lock unique_range_lock(current_range->m_lock);

            // If the current range still has space, add our record to it.
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

            unique_range_lock.unlock();
        }

        // Check the next range.
        // This is guaranteed to exist because of the above logic.
        ASSERT_INVARIANT(current_range->next_range() != nullptr, "There is no next range in the record list!");
        current_range = current_range->next_range();
    }
}

// Seek for the first valid record starting from the current iterator position.
void record_list_t::seek(record_iterator_t& iterator)
{
    while (!iterator.at_end())
    {
        // Search for a non-deleted record in the current range.
        for (; iterator.current_index < iterator.current_range->m_next_available_index; iterator.current_index++)
        {
            if (iterator.current_range->get(iterator.current_index).locator != c_invalid_gaia_locator)
            {
                return;
            }
        }

        // Release the lock on the current range before acquiring one on the next range.
        iterator.current_range->m_lock.unlock_shared();

        iterator.current_range = iterator.current_range->next_range();
        iterator.current_index = 0;

        // Acquire a lock on the next range if one exists.
        if (iterator.current_range != nullptr)
        {
            iterator.current_range->m_lock.lock_shared();
        }
    }
}

bool record_list_t::start(record_iterator_t& iterator)
{
    // Store a reference to itself in the iterator.
    iterator.record_list = this;

    // Position iterator to beginning of list.
    iterator.current_range = m_record_ranges;
    iterator.current_index = 0;

    // Schedule a compaction if enough records were deleted.
    if (m_approximate_count_deletions > m_range_size)
    {
        compact();

        // This is where the count can become inaccurate,
        // by missing the deletions that occur during compaction.
        // Hence it being called an approximate count.
        m_approximate_count_deletions = 0;
    }

    // Iterators will maintain a shared lock on the current range.
    iterator.current_range->m_lock.lock_shared();

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

    // Seek first valid record.
    seek(iterator);

    return (iterator.at_end() == false);
}

record_data_t record_list_t::get_record_data(record_iterator_t& iterator)
{
    ASSERT_PRECONDITION(iterator.at_end() == false, "Attempt to access invalid iterator state!");

    return iterator.current_range->get(iterator.current_index);
}

void record_list_t::mark_record_data_as_deleted(record_iterator_t& iterator)
{
    ASSERT_PRECONDITION(iterator.at_end() == false, "Attempt to access invalid iterator state!");

    iterator.current_range->get(iterator.current_index).locator = c_invalid_gaia_locator;
    iterator.current_range->m_has_deletions = true;

    iterator.record_list->m_approximate_count_deletions++;
}
