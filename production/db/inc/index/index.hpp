/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <ostream>
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

/**
* Reflection based logical key for the index.
*/
class index_key_t
{
    friend std::ostream& operator<<(std::ostream& os, const index_key_t& key);
    friend struct index_key_hash;

public:
    index_key_t() = default;

    template <typename... T_keys>
    index_key_t(T_keys... keys);

    template <typename T_key, typename... T_keys>
    void multi_insert(T_key key_value, T_keys... rest);

    template <typename T_key>
    void multi_insert(T_key key_value);

    void insert(gaia::db::payload_types::data_holder_t value);

    bool operator<(const index_key_t& other) const;
    bool operator<=(const index_key_t& other) const;
    bool operator>(const index_key_t& other) const;
    bool operator>=(const index_key_t& other) const;
    bool operator==(const index_key_t& other) const;

    size_t size() const;

private:
    int compare(const index_key_t& other) const;

private:
    std::vector<gaia::db::payload_types::data_holder_t> m_key_values;
};

/**
* Standard conforming hash function for index keys.
*/
struct index_key_hash
{
    gaia::db::payload_types::data_hash_t operator()(const index_key_t& key) const;
};

/**
* RAII object class with an XLOCK on the underlying index object.
* This interface is temporary until we have "live" indexes.
* T_structure is the underlying index data structure.
*/
template <typename T_structure>
class index_writer_guard_t
{
public:
    index_writer_guard_t(base_index_t* db_idx, T_structure& index)
        : m_db_idx(db_idx), m_data(index)
    {
        m_db_idx->get_lock().lock();
    }

    T_structure& get_index()
    {
        return m_data;
    }

    ~index_writer_guard_t()
    {
        m_db_idx->get_lock().unlock();
    }

private:
    base_index_t* m_db_idx;
    T_structure& m_data;
};

/**
 * Abstract in-memory index type:
 * T_structure is the underlying backing data structure of the index.
 * T_iterator is the iterator returned by the structure.
 */
template <typename T_structure, typename T_iterator>
class index_t : public base_index_t
{
public:
    index_t(gaia::common::gaia_id_t index_id, catalog::index_type_t index_type)
        : base_index_t(index_id, index_type)
    {
    }
    virtual ~index_t() override = default;

    virtual T_iterator begin() = 0;
    virtual T_iterator end() = 0;
    virtual T_iterator find(const index_key_t& key) = 0;

    virtual std::pair<T_iterator, T_iterator> equal_range(const index_key_t& key) = 0;

    // Index structure maintenance.
    void insert_index_entry(index_key_t&& key, index_record_t record);

    // Clear index structure.
    void clear() override;

    // RAII class for bulk index maintenance operations.
    index_writer_guard_t<T_structure> get_writer();

    std::function<std::optional<index_record_t>()> generator(gaia_txn_id_t txn_id) override;

protected:
    T_structure m_data;

private:
    gaia::common::gaia_id_t m_index_id;
    catalog::index_type_t m_index_type;

    // Find physical key corresponding to a logical_key + record or return the end iterator.
    // Returns the iterator type of the underlying structure.
    typename T_structure::iterator find_physical_key(index_key_t& key, index_record_t& record);
};

#include "index.inc"

} // namespace index
} // namespace db
} // namespace gaia
