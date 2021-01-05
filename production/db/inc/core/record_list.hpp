/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <list>
#include <shared_mutex>
#include <vector>

#include "db_types.hpp"
#include "gaia_db_internal.hpp"

namespace gaia
{
namespace db
{
namespace storage
{

struct record_data_t
{
    // Provides the record's locator.
    gaia::db::gaia_locator_t locator;

    // Tells whether the record has been deleted.
    bool is_deleted;

    record_data_t();

    void set(gaia::db::gaia_locator_t locator);
};

struct record_iterator_t;
class record_list_t;

class record_range_t
{
    friend struct record_iterator_t;
    friend class record_list_t;

public:
    record_range_t(size_t range_size);
    ~record_range_t();

    // Tells whether the range is full.
    bool is_full() const;

    // Compact a range by removing deleted entries.
    void compact();

    // Add and get an element.
    void add(gaia::db::gaia_locator_t locator);
    record_data_t& get(size_t index);

    // Add and read the next range.
    void add_next_range();
    record_range_t* next_range();

protected:
    size_t m_range_size;

    // The record range: an array of record_data_t structures.
    record_data_t* m_record_range;

    // The next available index in the array.
    size_t m_next_available_index;

    // Tells whether some of the range's entries have been deleted.
    bool m_has_deletions;

    // Pointer to the next range.
    record_range_t* m_next_range;

    // A lock for synchronizing operations on a range.
    mutable std::shared_mutex m_lock;
};

struct record_iterator_t
{
    // The position of the iterator is represented
    // by the current range and current index in the range.
    record_range_t* current_range;
    size_t current_index;

    record_iterator_t();
    ~record_iterator_t();

    // Tells whether the iterator position represents the end of the range.
    bool at_end();
};

class record_list_t
{
public:
    record_list_t(size_t range_size);
    ~record_list_t();

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
    static gaia::db::gaia_locator_t get_record_locator(record_iterator_t& iterator);

    // Mark the record currently referenced by the iterator as deleted.
    static void delete_record(record_iterator_t& iterator);

protected:
    size_t m_range_size;

    record_range_t* m_record_ranges;

    // Seek the first valid record starting from the current iterator position.
    // This may be the current iterator position, if it references a valid
    // (not deleted) record.
    static void seek(record_iterator_t& iterator);
};

} // namespace storage
} // namespace db
} // namespace gaia
