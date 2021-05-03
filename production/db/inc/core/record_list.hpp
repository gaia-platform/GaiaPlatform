/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <list>
#include <shared_mutex>
#include <vector>

#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

namespace gaia
{
namespace db
{
namespace storage
{

// A simple struct for packaging the data we track for each record.
// At the moment, this only consists of the record locator.
struct record_data_t
{
    // Provides the record's locator.
    gaia::db::gaia_locator_t locator;

    record_data_t();
};

struct record_iterator_t;
class record_list_t;

// A record list is implemented as a list of "ranges" that are arrays of record_data_t instances.
// This class implements a range.
class record_range_t
{
    friend struct record_iterator_t;
    friend class record_list_t;

public:
    record_range_t(size_t range_size);
    ~record_range_t();

    // Compact a range by removing deleted entries.
    void compact();

    // Add and get an element.
    void add(gaia::db::gaia_locator_t locator);
    record_data_t& get(size_t index);

    // Add and read the next range.
    void add_next_range();
    inline record_range_t* next_range()
    {
        return m_next_range;
    }

    inline bool has_deletions()
    {
        return m_has_deletions;
    }

    // Tells whether the range is full.
    inline bool is_full() const
    {
        return m_next_available_index >= m_range_size;
    }

protected:
    size_t m_range_size;

    // The record range: an array of record_data_t structures.
    record_data_t* m_record_range;

    // The next available index in the array.
    std::atomic<size_t> m_next_available_index;

    // Tells whether some of the range's entries have been deleted.
    std::atomic<bool> m_has_deletions;

    // Pointer to the next range.
    record_range_t* m_next_range;

    // A lock for synchronizing operations on a range.
    mutable std::shared_mutex m_lock;
};

// A struct that is used for maintaining the state of an iteration over a record list.
// An iterator's destructor will release any locks held during iteration.
struct record_iterator_t
{
    // A pointer to the record_list over which we are iterating.
    record_list_t* record_list;

    // The position of the iterator is represented
    // by the current range and the current index in the range.
    record_range_t* current_range;
    size_t current_index;

    record_iterator_t();
    ~record_iterator_t();

    // Tells whether the iterator position represents the end of the range.
    inline bool at_end()
    {
        return current_range == nullptr;
    }
};

// The implementation of a record list.
//
// Note: currently, there is no scenario for shrinking a list. This means that
// we don't expect ranges to disappear from a list.
//
// The synchronization of the access to the record list works as follows:
// - Additions will take an exclusive lock on the first range they find with available space.
// - Iterations will take a shared lock on the current range. Entries are marked as deleted during iterations.
// - Compaction (of entries within a range) will take an exclusive lock on the ranges that can be compacted.
class record_list_t
{
    friend struct record_iterator_t;

public:
    record_list_t(size_t range_size);
    ~record_list_t();

    // Reset the list to an empty list that has a single range allocated.
    // This method expects the record_list instance to no longer be in use by any other threads.
    // There is no synchronization to protect this operation from other concurrent access.
    void reset();

    // Compact the ranges of the list by removing deleted entries.
    void compact();

    // Add a recod's locator to our record list.
    void add(gaia::db::gaia_locator_t locator);

    // Start an iteration.
    // Return true if the iterator was positioned on a valid record
    // and false otherwise.
    bool start(record_iterator_t& iterator);

    // Advance the iterator to the next record.
    // Return true if the iterator was positioned on a valid record
    // and false otherwise.
    static bool move_next(record_iterator_t& iterator);

    // Read the record locator for the current iterator position.
    static record_data_t get_record_data(record_iterator_t& iterator);

    // Mark the record currently referenced by the iterator as deleted.
    static void mark_record_data_as_deleted(record_iterator_t& iterator);

protected:
    void clear();

protected:
    size_t m_range_size;

    record_range_t* m_record_ranges;

    // This approximate count of deletions is only used
    // to determine when to schedule the next list compaction.
    std::atomic<size_t> m_approximate_count_deletions;

    // Seek the first valid record starting from the current iterator position.
    // This may be the current iterator position, if it references a valid
    // (not deleted) record.
    static void seek(record_iterator_t& iterator);
};

} // namespace storage
} // namespace db
} // namespace gaia
