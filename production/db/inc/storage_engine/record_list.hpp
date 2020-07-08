/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <list>
#include <vector>

#include <synchronization.hpp>

namespace gaia
{
namespace db
{
namespace storage
{

struct record_data_t
{
    // Provides the record's locator.
    uint64_t locator;

    // Tells whether the record has been deleted.
    bool is_deleted;

    record_data_t();

    void set(uint64_t locator);
};

class record_list_t;

class record_range_t
{
    friend class record_list_t;

public:

    record_range_t();
    ~record_range_t();

    // Tells whether the range is full.
    bool is_full();

    // Compact a range by removing deleted entries.
    void compact();

    // Add and get an element.
    void add(uint64_t locator);
    record_data_t& get(size_t index);

    // Add and read the next range.
    void add_next_range();
    record_range_t* next_range();

protected:

    static const uint64_t c_range_size;

    // The record range: an array of record_data_t structures.
    record_data_t* m_record_range;

    // The next available index in the array.
    size_t m_next_available_index;

    // Tells whether some of the range's entries have been deleted.
    bool m_has_deletions;

    // Pointer to the next range.
    record_range_t* m_next_range;

    // A lock for synchronizing operations on a range.
    gaia::common::shared_mutex_t m_lock;
};

struct record_iterator_t
{
    // The position of the iterator is represented
    // by the current range and current index in the range.
    record_range_t* current_range;
    size_t current_index;

    record_iterator_t();

    // Tells whether the iterator position represents the end of the range.
    bool at_end();
};

class record_list_t
{
public:

    record_list_t();
    ~record_list_t();

    // Compact the ranges of the list by removing deleted entries.
    void compact();

    // Add a recod's locator to our record list.
    void add(uint64_t locator);

    // Start an iteration.
    // Return true if the iterator was positioned on a valid record
    // and false otherwise.
    bool start(record_iterator_t& iterator);

    // Advance the iterator to the next record.
    // Return true if the iterator was positioned on a valid record
    // and false otherwise.
    bool move_next(record_iterator_t& iterator);

    // Read the record locator for the current iterator position.
    uint64_t get_record_locator(record_iterator_t& iterator);

    // Mark the record currently referenced by the iterator as deleted.
    void delete_record(record_iterator_t& iterator);

protected:

    record_range_t* m_record_ranges;

    // Seek the first valid record starting from the current iterator position.
    // This may be the current iterator position, if it references a valid
    // (not deleted) record.
    static void seek(record_iterator_t& iterator);
};

}
}
}
