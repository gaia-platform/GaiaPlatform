////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <array>
#include <utility>

#include "gaia_internal/db/db_types.hpp"

#include "base_index.hpp"
#include "index_generator.hpp"
#include "index_key.hpp"

namespace gaia
{
namespace db
{
namespace index
{

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
        : m_index_lock(db_idx->get_lock()), m_db_idx(db_idx), m_data(index)
    {
    }

    T_structure& get_index()
    {
        return m_data;
    }

    // Bulk iterators
    std::pair<typename T_structure::iterator, typename T_structure::iterator> equal_range(const index_key_t& key);

private:
    std::unique_lock<std::recursive_mutex> m_index_lock;
    base_index_t* m_db_idx;
    T_structure& m_data;
};

constexpr size_t c_offset_buffer_size = 32;

/**
 * Buffer storing data for garbage collecting offsets.
 */
class index_offset_buffer_t
{
public:
    inline void insert(gaia_offset_t offset, common::gaia_type_t type);
    inline gaia_offset_t get_offset(size_t index) const;
    inline common::gaia_type_t get_type(size_t index) const;
    inline size_t size() const;
    inline bool empty() const;

private:
    std::array<std::pair<gaia_offset_t, common::gaia_type_t>, c_offset_buffer_size> m_offsets = {};
    size_t m_size = 0;
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
    index_t(gaia::common::gaia_id_t index_id, catalog::index_type_t index_type, index_key_schema_t key_schema, bool is_unique)
        : base_index_t(index_id, index_type, key_schema, is_unique)
    {
    }
    ~index_t() override = default;

    virtual T_iterator begin() = 0;
    virtual T_iterator end() = 0;
    virtual T_iterator find(const index_key_t& key) = 0;

    virtual std::pair<T_iterator, T_iterator> equal_range(const index_key_t& key) = 0;

    // Index structure maintenance.
    void insert_index_entry(index_key_t&& key, index_record_t record);
    void remove_index_entry_with_offset(const index_key_t& key, gaia_offset_t offset, gaia_txn_id_t gc_txn_id);

    // This method will mark all entries below a specified txn_id as committed.
    // This must only be called after all aborted/terminated index entries below the txn_id are garbage collected.
    void mark_entries_committed(gaia_txn_id_t metadata_truncation_txn_id);

    // Clear index structure.
    void clear() override;

    // RAII class for bulk index maintenance operations.
    index_writer_guard_t<T_structure> get_writer();

    std::shared_ptr<common::iterators::generator_t<index_record_t>> generator(gaia_txn_id_t txn_id) override;
    std::shared_ptr<common::iterators::generator_t<index_record_t>> equal_range_generator(
        gaia_txn_id_t txn_id, std::vector<char>&& key_storage, const index_key_t& key) override;

protected:
    T_structure m_data;

private:
    // Find physical key corresponding to a logical_key + record or return the end iterator.
    // Returns the iterator type of the underlying structure.
    typename T_structure::iterator find_physical_key(const index_key_t& key, const index_record_t& record);

    // The following txn_ids are internal markers to determine if any work needs to be done for garbage collection.
    //
    // last_updated_txn_id - txn_id where the index was last updated/modified.
    // last_mark_committed_txn_id - the txn_id below which all index entries have been marked committed.
    gaia_txn_id_t m_last_updated_txn_id;
    gaia_txn_id_t m_last_mark_committed_txn_id;
};

#include "index.inc"

} // namespace index
} // namespace db
} // namespace gaia
