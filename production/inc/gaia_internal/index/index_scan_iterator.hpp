/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia/common.hpp"
#include "client_index_impl.hpp"
#include "db_types.hpp"
#include "hash_index.hpp"
#include "index.hpp"
#include "index_builder.hpp"
#include "range_index.hpp"
#include "se_helpers.hpp"
#include "se_object.hpp"
#include "visibility.hpp"

// Actual heart of the index scan, visibility resolution etc... happens here
namespace gaia
{
namespace db
{
namespace index
{

template <typename T_iter>
class index_scan_iterator_t
{
private:
    T_iter local_iter;
    T_iter local_iter_end;
    index_stream_iterator_t remote_iter;

    gaia::common::gaia_id_t index_id;
    gaia_locator_t locator;

    void init();
    void next_visible_loc();
    bool valid_record(const index_record_t& record) const;
    bool is_visible(const index_record_t& record) const;
    index_key_t record_to_key(const index_record_t& record) const;
    const se_object_t* to_ptr() const;
    bool select_local_for_merge() const;
    void advance_remote();
    void advance_local();
    bool remote_end() const;
    bool local_end() const;
    bool has_more() const;

public:
    index_scan_iterator_t();
    index_scan_iterator_t(gaia::common::gaia_id_t index_id)
        : index_id(index_id)
    {
        init();
    }
    ~index_scan_iterator_t();

    index_scan_iterator_t& operator++();
    index_scan_iterator_t& operator++(int);

    const se_object_t& operator*() const;
    se_object_t& operator*();
    const se_object_t* operator->() const;
    se_object_t* operator->();

    bool operator==(const index_scan_iterator_t&) const;
    bool operator!=(const index_scan_iterator_t&) const;
    bool operator!=(std::nullptr_t) const;
};

template <typename T_iter>
void index_scan_iterator_t<T_iter>::advance_local()
{
    do
    {
        ++local_iter;
    } while (!local_end() && !is_visible(*local_iter));
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::local_end() const
{
    return local_iter == local_iter_end;
}

template <typename T_iter>
void index_scan_iterator_t<T_iter>::advance_remote()
{
    do
    {
        ++remote_iter;
    } while (!remote_end() && !valid_record(*remote_iter) && !is_visible(*remote_iter));
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::remote_end() const
{
    return static_cast<bool>(remote_iter);
}

template <typename T_iter>
index_scan_iterator_t<T_iter>& index_scan_iterator_t<T_iter>::operator++()
{
    next_visible_loc();
    return *this;
}

template <typename T_iter>
index_scan_iterator_t<T_iter>& index_scan_iterator_t<T_iter>::operator++(int)
{
    auto tmp = *this;
    next_visible_loc();
    return tmp;
}

template <typename T_iter>
const se_object_t& index_scan_iterator_t<T_iter>::operator*() const
{
    return *to_ptr();
}

template <typename T_iter>
const se_object_t* index_scan_iterator_t<T_iter>::operator->() const
{
    return to_ptr();
}

template <typename T_iter>
se_object_t* index_scan_iterator_t<T_iter>::operator->()
{
    return const_cast<se_object_t*>(const_cast<index_scan_iterator_t<T_iter>*>(this)->to_ptr());
}

template <typename T_iter>
se_object_t& index_scan_iterator_t<T_iter>::operator*()
{
    return *const_cast<se_object_t*>(const_cast<index_scan_iterator_t<T_iter>*>(this)->to_ptr());
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::operator==(const index_scan_iterator_t<T_iter>& other) const
{
    return index_id == other.index_id && locator == other.locator;
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::operator!=(const index_scan_iterator_t<T_iter>& other) const
{
    return !(*this == other);
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::operator!=(std::nullptr_t) const
{
    return has_more();
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::select_local_for_merge() const
{
    return true;
}

// Ordering matters for range iterators.
template <>
bool index_scan_iterator_t<range_index_iterator_t>::select_local_for_merge() const
{
    return record_to_key(local_iter->second) <= record_to_key(*remote_iter);
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::has_more() const
{
    return !remote_end() && !local_end();
}
template <typename T_iter>
index_key_t index_scan_iterator_t<T_iter>::record_to_key(const index_record_t& record) const
{
    auto obj = locator_to_ptr(record.locator);
    return index_builder::make_key(index_id, reinterpret_cast<const uint8_t*>(obj->payload));
}

// checks visibility of the record vs the current txn
template <typename T_iter>
bool index_scan_iterator_t<T_iter>::valid_record(const index_record_t& record) const
{
    // Check the if the record's txn is below the begin_ts.
    // This is a very loose upper bound as those transactions might not be committed in our snapshot.
    // A more correct check would be to check if the record's txn_id has a commit_ts below our begin_ts
    // but this information is not available to the client.
    //
    // NOTE: for v1 filtering on the snapshot is a possibility on the stream from the server,
    // but v2 will have shmem indexes, where server-side filtering is not possible.

    return get_begin_ts() > record.txn_id;
}

// checks visibility of locator in the record
template <typename T_iter>
bool index_scan_iterator_t<T_iter>::is_visible(const index_record_t& record) const
{
    if (record.deleted)
        return false;

    return is_visible(record.locator, record.offset);
}

template <typename T_iter>
const se_object_t* index_scan_iterator_t<T_iter>::to_ptr() const
{
    return locator_to_ptr(locator);
}

template <typename T_iter>
void index_scan_iterator_t<T_iter>::next_visible_loc()
{
    if (local_end() && !remote_end())
    {
        locator = (*remote_iter).locator;
        advance_remote();
    }
    else if (!local_end() && !remote_end())
    {
        if (select_local_for_merge())
        {
            locator = local_iter->second.locator;
            advance_local();
        }
        else
        {
            locator = (*remote_iter).locator;
            advance_remote();
        }
    }
    else if (remote_end() && !local_end())
    {
        locator = (*remote_iter).locator;
        advance_local();
    }
    else
    {
        locator = c_invalid_gaia_locator;
    }
}

template <typename T_iter>
void index_scan_iterator_t<T_iter>::init()
{
    // advance both iterators to first visible record
    if (!remote_end() && !is_visible(*remote_iter))
    {
        advance_remote();
    }
    if (!local_end() && !is_visible(local_iter.second))
    {
        advance_local();
    }
    // assign the first visible locator
    next_visible_loc();
}

} // namespace index
} // namespace db
} // namespace gaia
