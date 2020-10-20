/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <list>
#include <vector>

#include "gaia_internal/db/db_types.hpp"

#include "base_index.hpp"
#include "data_holder.hpp"

namespace gaia
{
namespace db
{
namespace index
{

/*
* Reflection based logical key for the index.
*/

class index_key_t
{
    friend struct index_key_hash;
    std::vector<gaia::db::payload_types::data_holder_t> key_values;

private:
    int compare(const index_key_t& other) const;

public:
    index_key_t() = default;

    template <typename... Ts>
    index_key_t(Ts... keys)
    {
        multi_insert(keys...);
    }

    void insert(gaia::db::payload_types::data_holder_t value);

    template <typename T, typename... Ts>
    void multi_insert(T key_value, Ts... rest)
    {
        insert(key_value);
        multi_insert(rest...);
    }

    template <typename T>
    void multi_insert(T key_value)
    {
        insert(key_value);
    }

    bool operator<(const index_key_t& other) const;
    bool operator<=(const index_key_t& other) const;
    bool operator>(const index_key_t& other) const;
    bool operator>=(const index_key_t& other) const;
    bool operator==(const index_key_t& other) const;

    size_t size() const;
};

/*
* Standard conforming hash function for index keys.
*/

struct index_key_hash
{
    gaia::db::payload_types::data_hash_t operator()(const index_key_t& key) const;
};

/*
* RAII object class with an XLOCK on the underlying index object
* This interface is temporary until we have "live" indexes
*/

template <typename T>
class index_writer_t
{
private:
    base_index_t* db_idx;
    T& index;

public:
    index_writer_t(base_index_t* db_idx, T& index)
        : db_idx(db_idx), index(index)
    {
        db_idx->get_lock().lock();
    }

    T& get_index()
    {
        return index;
    }

    ~index_writer_t()
    {
        db_idx->get_lock().unlock();
    }
};

/*
 * Abstract in-memory index type
 * T is the underlying backing structure
 * T_iterator is the iterator returned by the structure
 */

template <typename T, typename T_iterator>
class index_t : public base_index_t
{
private:
    gaia::common::gaia_id_t index_id;
    index_type_t index_type;

protected:
    T index;

    // find physical key corresponding to a logical_key + record or return the end iterator.
    auto find_physical_key(index_key_t& key, index_record_t& record)
    {
        auto idx_rec = index.find(key);
        while (idx_rec != index.end() && idx_rec->first == key)
        {
            if (idx_rec->second.locator == record.locator && idx_rec->second.txn_id == record.txn_id)
            {
                return idx_rec;
            }
            ++idx_rec;
        }

        return index.end();
    }

public:
    index_t(gaia::common::gaia_id_t index_id, index_type_t index_type)
        : base_index_t(index_id, index_type)
    {
    }

    virtual T_iterator begin() = 0;
    virtual T_iterator end() = 0;
    virtual T_iterator find(const index_key_t& key) = 0;

    virtual std::pair<T_iterator, T_iterator> equal_range(const index_key_t& key) = 0;

    // Index structure maintenance
    void insert_index_entry(index_key_t&& key, index_record_t record)
    {
        std::lock_guard lock(index_lock);
        auto old_it = find_physical_key(key, record);
        if (old_it != index.end())
        {
            // Update old record matching this physical key
            old_it->second = record;
        }
        else
        {
            index.insert(std::make_pair(key, record));
        }
    }

    // Clear index structure
    void clear()
    {
        std::lock_guard lock(index_lock);
        index.clear();
    }

    // RAII class for bulk index maintenance operations
    index_writer_t<T> get_writer()
    {
        return index_writer_t<T>(this, index);
    };

    virtual ~index_t() = 0;
}; // namespace db

template <typename T, typename T_iterator>
index_t<T, T_iterator>::~index_t()
{
    // Blank definition for pure virtual dtor
}

} // namespace index
} // namespace db
} // namespace gaia
