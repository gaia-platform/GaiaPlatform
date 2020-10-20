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
* Reflection based index_key type.
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
    std::size_t operator()(const index_key_t& key) const;
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
    value_index_type_t index_type;

protected:
    T index;

public:
    index_t(gaia::common::gaia_id_t index_id, value_index_type_t index_type)
        : base_index_t(index_id, index_type)
    {
    }

    virtual T_iterator begin() = 0;
    virtual T_iterator end() = 0;
    virtual T_iterator find(const index_key_t& key) = 0;

    virtual std::pair<T_iterator, T_iterator> equal_range(const index_key_t& key) = 0;

    // index structure maintenance
    virtual void insert_index_entry(index_key_t&& key, index_record_t record) = 0;
    // TODO: define interface for gc of index entries, likely it is its own strategy object

    virtual ~index_t() = 0;
};

template <typename T, typename T_iterator>
index_t<T, T_iterator>::~index_t()
{
    // Blank definition for pure virtual dtor
}

} // namespace index
} // namespace db
} // namespace gaia
