/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <list>
#include <shared_mutex>
#include <vector>

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/queue.hpp"
#include "gaia_internal/db/db_types.hpp"

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
    record_data_t(gaia::db::gaia_locator_t locator);
};

struct record_iterator_t;
class record_list_t;

// A record list is implemented as a list of "ranges" that are arrays of record_data_t instances.
// This class implements a range.
// The methods of this class take no locks.
class record_range_t
{
    friend struct record_iterator_t;
    friend class record_list_t;

public:
    record_range_t(record_list_t* record_list);
    ~record_range_t();

    // Compact a range by removing deleted entries.
    void compact();

    // Add and get an element.
    // The addition may fail when it's running concurrently,
    // so its return value indicates whether it succeeded or not.
    bool add(const record_data_t& record_data);
    record_data_t& get(size_t index);

    // Add and read the next range.
    void add_next_range();
    inline record_range_t* next_range();

    // Tells whether the range is full.
    inline bool is_full() const;

protected:
    void clear();

protected:
    // A back pointer to the record_list that the range belongs to.
    record_list_t* m_record_list;

    // The record range: an array of record_data_t structures.
    record_data_t* m_record_range;

    // The next available index in the array.
    std::atomic<size_t> m_next_available_index;

    // Tells whether some of the range's entries have been marked as deleted.
    std::atomic<bool> m_has_deletions;

    // Pointer to the next range.
    std::atomic<record_range_t*> m_next_range;

    // A lock for synchronizing operations on the range.
    mutable std::shared_mutex m_lock;
};

// A struct that is used for maintaining the state of an iteration over a record list.
// An iterator's destructor will release any locks held during iteration.
struct record_iterator_t
{
    record_iterator_t();
    record_iterator_t(record_iterator_t&&) noexcept;
    ~record_iterator_t();

    // These custom copy operations ensure that only uninitialized iterators can be copied,
    // by asserting in any other situation.
    record_iterator_t(const record_iterator_t&);
    record_iterator_t& operator=(const record_iterator_t&);

    // Tells whether the iterator position represents the end of the iteration.
    inline bool at_end();

    // The position of the iterator is represented
    // by the current range and the current index in the range.
    record_range_t* current_range;
    size_t current_index;
};

// The implementation of a record list.
//
// Note: currently, there is no scenario for shrinking a list. This means that
// we don't expect ranges to disappear from a list.
//
// The synchronization of the access to the record list works as follows:
// - Additions will take a shared lock on the first range they find with available space
// (or on the last range, if the list is full).
// - Iterations will take a shared lock on the current range. Entries are marked as deleted during iterations.
// - Compaction (of entries within a range) will take an exclusive lock on the ranges that can be compacted.
//
// NOTE: An additional compaction could be implemented for removing unnecessary ranges.
// It synchronize using a global lock at list level.
// This is not done now because of the current plan of reimplementing all these structures in shared memory.
class record_list_t
{
    friend struct record_iterator_t;
    friend class record_range_t;

protected:
    // How many extra elements than needed should we wait to accumulate in a queue
    // before extracting them, to prevent locking conflicts with additions.
    const size_t c_dequeue_extra_buffer_size = 2;

public:
    record_list_t(size_t range_size);
    ~record_list_t();

    // Reset the list to an empty list that has a single range allocated.
    // This method expects the record_list instance to no longer be in use by any other threads.
    // There is no synchronization to protect this operation from other concurrent access.
    void reset(size_t range_size);

    // Compact the ranges of the list by removing deleted entries.
    void compact();

    // Add a record's data to our record list.
    void add(gaia::db::gaia_locator_t locator);

    // Request the deletion of a record entry.
    void request_deletion(gaia::db::gaia_locator_t locator);

    // Get the size of a range in this list.
    inline size_t get_range_size();

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
    void delete_list();

    // Mark records as deleted.
    void perform_deletion_marking(size_t deletion_batch_size);

    // Seek the first valid record starting from the current iterator position.
    // This may be the record at the current iterator position, if that position
    // references a valid (not marked as deleted) record.
    static void seek(record_iterator_t& iterator);

protected:
    size_t m_range_size;

    record_range_t* m_record_ranges;

    gaia::common::mpsc_queue_t<gaia::db::gaia_locator_t> m_deletions_requested;

    // These booleans are used to ensure that only one operation can be attempted at a time.
    std::atomic<bool> m_is_deletion_marking_in_progress;
    std::atomic<bool> m_is_compaction_in_progress;

    // This count of deletion markings is only used
    // to determine when to schedule the next list compaction.
    std::atomic<size_t> m_count_deletion_markings;
};

#include "record_list.inc"

} // namespace storage
} // namespace db
} // namespace gaia
