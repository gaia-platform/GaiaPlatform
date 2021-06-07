/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "record_list.hpp"

#include <unordered_map>

#include "gaia_internal/common/retail_assert.hpp"

using namespace std;

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::db::storage;

record_data_t::record_data_t()
{
    locator = c_invalid_gaia_locator;
}

record_data_t::record_data_t(gaia_locator_t locator)
{
    this->locator = locator;
}

record_range_t::record_range_t(record_list_t* record_list)
{
    ASSERT_PRECONDITION(record_list != nullptr, "Record list passed to record_range_t constructor must not be null!");

    clear();

    m_record_list = record_list;
    m_record_range = new record_data_t[m_record_list->get_range_size()];

    ASSERT_POSTCONDITION(
        m_record_range[0].locator == c_invalid_gaia_locator, "Record range is not properly initialized!");
    ASSERT_POSTCONDITION(
        m_record_range[m_record_list->get_range_size() - 1].locator == c_invalid_gaia_locator,
        "Record range is not properly initialized!");
}

record_range_t::~record_range_t()
{
    delete[] m_record_range;
    clear();
}

void record_range_t::clear()
{
    m_record_list = nullptr;
    m_record_range = nullptr;
    m_next_available_index = 0;
    m_has_deletions = false;
    m_next_range = nullptr;
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
        else
        {
            m_record_list->m_count_deletion_markings--;
        }
    }

    m_next_available_index = write_index;
    m_has_deletions = false;

    ASSERT_POSTCONDITION(
        m_next_available_index < m_record_list->get_range_size(),
        "Compaction has not made any space available in the range!");
}

bool record_range_t::add(const record_data_t& record_data)
{
    ASSERT_PRECONDITION(
        record_data.locator != c_invalid_gaia_locator, "An invalid locator was passed to record_range_t::add()!");

    size_t index = m_next_available_index++;
    if (index < m_record_list->get_range_size())
    {
        m_record_range[index] = record_data;
        return true;
    }
    else
    {
        ASSERT_INVARIANT(
            m_next_available_index >= m_record_list->get_range_size(),
            "m_next_available_index should have indicated that no next element is available!");

        // We may have bumped this too far - reset it back to just past the last range element.
        m_next_available_index = m_record_list->get_range_size();
        return false;
    }
}

record_data_t& record_range_t::get(size_t index)
{
    ASSERT_PRECONDITION(m_record_range != nullptr, "Range is not allocated!");
    ASSERT_PRECONDITION(index < m_record_list->get_range_size(), "Range index is out of range bounds!");
    ASSERT_PRECONDITION(index < m_next_available_index, "Range index is out of used range bounds!");

    return m_record_range[index];
}

void record_range_t::add_next_range()
{
    auto next_range = new record_range_t(m_record_list);
    record_range_t* expected_next_range = nullptr;

    // Try to set the next range. If another threads succeeds before us,
    // just delete the range we created and continue.
    if (!m_next_range.compare_exchange_strong(expected_next_range, next_range))
    {
        delete next_range;
    }

    ASSERT_POSTCONDITION(m_next_range != nullptr, "The next range should have been set!");
}

record_iterator_t::record_iterator_t()
{
    current_range = nullptr;
    current_index = 0;
}

record_iterator_t::record_iterator_t(record_iterator_t&& other) noexcept
{
    current_range = other.current_range;
    current_index = other.current_index;
    other.current_range = nullptr;
    other.current_index = 0;
}

record_iterator_t::~record_iterator_t()
{
    // Release the shared lock maintained on the current range.
    if (current_range != nullptr)
    {
        current_range->m_lock.unlock_shared();
    }
}

record_iterator_t::record_iterator_t(const record_iterator_t& other)
{
    ASSERT_PRECONDITION(other.current_range == nullptr, "An attempt was made to copy an initialized record_iterator!");
    this->current_range = other.current_range;
    this->current_index = other.current_index;
}

record_iterator_t& record_iterator_t::operator=(const record_iterator_t& other)
{
    ASSERT_PRECONDITION(this->current_range == nullptr, "An attempt was made to copy into an initialized record_iterator!");
    ASSERT_PRECONDITION(other.current_range == nullptr, "An attempt was made to copy an initialized record_iterator!");
    this->current_range = other.current_range;
    this->current_index = other.current_index;
    return *this;
}

record_list_t::record_list_t(size_t range_size)
{
    clear();
    reset(range_size);
}

record_list_t::~record_list_t()
{
    delete_list();
}

void record_list_t::clear()
{
    m_range_size = 0;
    m_record_ranges = nullptr;
    m_is_deletion_marking_in_progress = false;
    m_is_compaction_in_progress = false;
    m_count_deletion_markings = 0;
}

void record_list_t::delete_list()
{
    // Iterate over the range list and deallocate each range.
    while (m_record_ranges != nullptr)
    {
        record_range_t* current_range = m_record_ranges;
        m_record_ranges = current_range->next_range();
        delete current_range;
    }

    ASSERT_INVARIANT(m_record_ranges == nullptr, "Failed to clear record_list!");

    clear();
}

void record_list_t::reset(size_t range_size)
{
    ASSERT_PRECONDITION(range_size > 0, "Range size must be greater than 0");

    // Delete record list.
    delete_list();

    m_range_size = range_size;
    m_record_ranges = new record_range_t(this);
}

void record_list_t::compact()
{
    // Iterate over our list and look for ranges that can be compacted.
    for (record_range_t* current_range = m_record_ranges;
         current_range != nullptr;
         current_range = current_range->next_range())
    {
        if (current_range->m_has_deletions)
        {
            // Take an exclusive lock before compacting the range.
            unique_lock unique_range_lock(current_range->m_lock);

            current_range->compact();
        }
    }
}

void record_list_t::add(gaia_locator_t locator)
{
    ASSERT_PRECONDITION(
        locator != c_invalid_gaia_locator, "An invalid locator was passed to record_list_t::add()!");

    record_data_t record_data(locator);

    // We'll iterate over our list and look for a range with available space.
    for (record_range_t* current_range = m_record_ranges;
         current_range != nullptr;
         current_range = current_range->next_range())
    {
        // Only take a lock on the current range if it still has space available
        // or if it's the last range and we need to add a new one.
        if (!current_range->is_full() || current_range->next_range() == nullptr)
        {
            // Take a shared lock before attempting to add the new locator.
            // This is safe because the operations that we perform here on the range
            // are using atomic operations to ensure safety.
            shared_lock shared_range_lock(current_range->m_lock);

            // If the range is not full and we succeed the addition, we're done.
            if (!current_range->is_full() && current_range->add(record_data))
            {
                return;
            }

            // If we reached the last range, add a new one.
            if (current_range->next_range() == nullptr)
            {
                current_range->add_next_range();
            }

            shared_range_lock.unlock();
        }

        // The next range is guaranteed to exist because of the above logic.
        ASSERT_INVARIANT(current_range->next_range() != nullptr, "There is no next range in the record list!");
    }
}

void record_list_t::request_deletion(gaia_locator_t locator)
{
    ASSERT_PRECONDITION(
        locator != c_invalid_gaia_locator, "An invalid locator was passed to record_list_t::request_deletion()!");

    m_deletions_requested.enqueue(locator);

    // If we accumulated enough deletion requests, process a batch of them.
    // We batch deletions, because marking them requires a full record_list scan.
    size_t deletion_batch_size = m_range_size;
    if (m_deletions_requested.size() > deletion_batch_size + c_dequeue_extra_buffer_size
        && !m_is_deletion_marking_in_progress)
    {
        // Acquire the right to perform the deletion marking.
        // This prevents multiple threads from attempting to perform this operation at the same time.
        // Only the first thread that turns on the boolean will continue to do the work.
        bool expected_setting = false;
        if (m_is_deletion_marking_in_progress.compare_exchange_strong(expected_setting, true))
        {
            perform_deletion_marking(deletion_batch_size);

            m_is_deletion_marking_in_progress = false;
        }
    }
}

void record_list_t::perform_deletion_marking(size_t deletion_batch_size)
{
    // Extract locators from the queue and place them in a map, for quick lookup.
    std::unordered_map<gaia_locator_t, bool> map_locators;
    for (size_t count = 0; count < deletion_batch_size; ++count)
    {
        gaia_locator_t locator = c_invalid_gaia_locator;
        m_deletions_requested.dequeue(locator);

        ASSERT_INVARIANT(
            locator != c_invalid_gaia_locator, "An invalid locator was returned from the queue of requested deletions!");

        map_locators.insert(make_pair(locator, true));
    }

    // Iterate through the record list and mark all locators found in the map as deleted.
    record_iterator_t iterator;
    size_t count_deletion_markings = 0;
    for (start(iterator); !iterator.at_end(); move_next(iterator))
    {
        if (map_locators.find(get_record_data(iterator).locator) != map_locators.end())
        {
            mark_record_data_as_deleted(iterator);
            ++count_deletion_markings;
        }
    }

    ASSERT_POSTCONDITION(count_deletion_markings == deletion_batch_size, "Have failed to mark all requested deletions!");
}

// Seek the first valid record starting from the current iterator position.
void record_list_t::seek(record_iterator_t& iterator)
{
    while (!iterator.at_end())
    {
        // m_next_available_index can sometimes be bumped beyond the end of the range,
        // so we need to check it against the range size as well.
        size_t range_index_limit = std::min(
            iterator.current_range->m_next_available_index.load(),
            iterator.current_range->m_record_list->get_range_size());

        // Search for a non-deleted record in the current range.
        for (; iterator.current_index < range_index_limit; iterator.current_index++)
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
    // Position iterator to beginning of list.
    iterator.current_range = m_record_ranges;
    iterator.current_index = 0;

    // Schedule a compaction if enough records were deleted.
    if (m_count_deletion_markings > m_range_size
        && !m_is_compaction_in_progress)
    {
        // Acquire the right to perform the deletion marking.
        // This prevents multiple threads from attempting to perform this operation at the same time.
        // Only the first thread that turns on the boolean will continue to do the work.
        bool expected_setting = false;
        if (m_is_compaction_in_progress.compare_exchange_strong(expected_setting, true))
        {
            compact();

            m_is_compaction_in_progress = false;
        }
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

    // We mark a record data entry as deleted by setting its locator to an invalid value.
    iterator.current_range->get(iterator.current_index).locator = c_invalid_gaia_locator;
    iterator.current_range->m_has_deletions = true;

    iterator.current_range->m_record_list->m_count_deletion_markings++;
}
