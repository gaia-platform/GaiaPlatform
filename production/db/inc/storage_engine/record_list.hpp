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
    uint64_t locator;

    bool is_deleted;

    record_data_t();
};

class record_range_t
{
public:

    record_range_t();
    ~record_range_t();

    void compact();

    record_data_t get(size_t index);

protected:

    static const uint64_t c_range_size;

    record_data_t* m_record_range;

    size_t m_next_available_index;

    bool m_has_deletions;

    record_range_t* m_next_range;

    gaia::common::shared_mutex_t m_lock;
};

struct record_iterator_t
{
    record_range_t* current_range;
    size_t current_index;

    record_iterator_t();

    bool at_end();
};

class record_list_t
{
public:

    record_list_t();
    ~record_list_t();

    void add(uint64_t locator);

    record_iterator_t start();
    bool move_next(record_iterator_t& iterator);
    uint64_t get_record_locator(record_iterator_t& iterator);

protected:

    record_range_t* m_record_ranges;
};

}
}
}
