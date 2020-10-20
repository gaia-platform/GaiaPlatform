/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <memory>

#include "gaia/common.hpp"

//#include "client_index_impl.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/index/index_builder.hpp"

#include "db_helpers.hpp"
#include "hash_index.hpp"
#include "index.hpp"
#include "range_index.hpp"

// Actual heart of the index scan, visibility resolution etc... happens here
namespace gaia
{
namespace qp
{
namespace scan
{

struct base_index_scan_iterator_t
{
};

template <typename T_iter>
class index_scan_iterator_t : base_index_scan_iterator_t
{
private:
    T_iter local_iter;
    T_iter local_iter_end;
    //TODO: fix this interface! This should be the streaming iter *not* T_iter
    T_iter remote_iter;

    common::gaia_id_t index_id;
    db::gaia_locator_t locator;

    void init();
    void next_visible_loc();
    db::index::index_key_t record_to_key(const db::index::index_record_t& record) const;
    const db::db_object_t* to_ptr() const;
    bool select_local_for_merge() const;
    void advance_remote();
    void advance_local();
    bool remote_end() const;
    bool local_end() const;
    bool has_more() const;

public:
    index_scan_iterator_t();
    index_scan_iterator_t(common::gaia_id_t index_id)
        : index_id(index_id)
    {
        init();
    }
    ~index_scan_iterator_t();

    index_scan_iterator_t& operator++();
    index_scan_iterator_t& operator++(int);

    const db::db_object_t& operator*() const;
    db::db_object_t& operator*();
    const db::db_object_t* operator->() const;
    db::db_object_t* operator->();

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
    } while (!remote_end());
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
const db::db_object_t& index_scan_iterator_t<T_iter>::operator*() const
{
    return *to_ptr();
}

template <typename T_iter>
const db::db_object_t* index_scan_iterator_t<T_iter>::operator->() const
{
    return to_ptr();
}

template <typename T_iter>
db::db_object_t* index_scan_iterator_t<T_iter>::operator->()
{
    return const_cast<db::db_object_t*>(const_cast<index_scan_iterator_t<T_iter>*>(this)->to_ptr());
}

template <typename T_iter>
db::db_object_t& index_scan_iterator_t<T_iter>::operator*()
{
    return *const_cast<db::db_object_t*>(const_cast<index_scan_iterator_t<T_iter>*>(this)->to_ptr());
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

// Ordering matters for range iterators.
template <>
bool index_scan_iterator_t<db::index::range_index_iterator_t>::select_local_for_merge() const
{
    return record_to_key(local_iter->second) <= record_to_key(remote_iter->second);
}

// Ignore ordering for most iterators
template <typename T_iter>
bool index_scan_iterator_t<T_iter>::select_local_for_merge() const
{
    return true;
}

template <typename T_iter>
bool index_scan_iterator_t<T_iter>::has_more() const
{
    return !remote_end() && !local_end();
}
template <typename T_iter>
db::index::index_key_t index_scan_iterator_t<T_iter>::record_to_key(const db::index::index_record_t& record) const
{
    auto obj = db::locator_to_ptr(record.locator);
    return db::index::index_builder::make_key(index_id, obj->type, reinterpret_cast<const uint8_t*>(obj->payload));
}

template <typename T_iter>
const db::db_object_t* index_scan_iterator_t<T_iter>::to_ptr() const
{
    return db::locator_to_ptr(locator);
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
        locator = db::c_invalid_gaia_locator;
    }
}

template <typename T_iter>
void index_scan_iterator_t<T_iter>::init()
{
    // TODO: init socket setup for SCAN

    // TODO: rebuild local indexes on the client

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

std::unique_ptr<base_index_scan_iterator_t> get_index_scan_iterator(common::gaia_id_t index_id)
{
    // get type of index

    // cast and return
    return nullptr;
}

} // namespace scan
} // namespace qp
} // namespace gaia
