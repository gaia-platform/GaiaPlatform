/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "index_key.hpp"

namespace gaia
{
namespace db
{
namespace index
{

class index_not_found : public common::gaia_exception
{
public:
    explicit index_not_found(common::gaia_id_t index_id)
    {
        std::stringstream message;
        message << "Cannot find index '" << index_id << "'.";
        m_message = message.str();
    }
};

enum class index_record_operation_t : uint8_t
{
    not_set,

    insert,
    remove,
    // Updates that modify the index key will generate remove and insert records.
    // Updates that do not modify the index key will generate a single update record.
    update,
};

struct index_record_t
{
    // The following fields should occupy 3x64bit.
    gaia::db::gaia_txn_id_t txn_id;
    gaia::db::gaia_locator_t locator;
    gaia::db::gaia_offset_t offset;
    index_record_operation_t operation;
    uint8_t flags;

    // Returns true if the record has been initialized and false if it was not.
    // This is needed to enable the stream generator to produce instances of index_record_t.
    constexpr bool is_valid() const
    {
        return (operation != index_record_operation_t::not_set);
    }

    friend std::ostream& operator<<(std::ostream& os, const index_record_t& rec);
};

constexpr size_t c_index_record_packed_size{24};

// We use this assert to check that the index record structure is packed optimally.
static_assert(sizeof(index_record_t) == c_index_record_packed_size, "index_record_t size has changed unexpectedly!");

// Flags for index record.
constexpr size_t c_mark_committed_shift{0};
constexpr uint8_t c_mark_committed_mask{1 << c_mark_committed_shift};

constexpr bool is_marked_committed(const index_record_t& record)
{
    return (record.flags & c_mark_committed_mask) == c_mark_committed_mask;
}

constexpr void mark_committed(index_record_t& record)
{
    record.flags |= c_mark_committed_mask;
}

class base_index_t
{
public:
    base_index_t(gaia::common::gaia_id_t index_id, catalog::index_type_t index_type, index_key_schema_t key_schema, bool is_unique)
        : m_index_id(index_id), m_index_type(index_type), m_key_schema(key_schema), m_is_unique(is_unique)
    {
    }

    virtual ~base_index_t() = default;

    gaia::common::gaia_id_t id() const;
    catalog::index_type_t type() const;
    gaia::common::gaia_type_t table_type() const;
    const index_key_schema_t& key_schema() const;
    bool is_unique() const;

    std::recursive_mutex& get_lock() const;

    bool operator==(const base_index_t& other) const;

    virtual void clear() = 0;
    virtual std::shared_ptr<common::iterators::generator_t<index_record_t>> generator(gaia_txn_id_t txn_id) = 0;
    virtual std::shared_ptr<common::iterators::generator_t<index_record_t>> equal_range_generator(
        gaia_txn_id_t txn_id, std::vector<char>&& key_storage, const index_key_t& key)
        = 0;

protected:
    gaia::common::gaia_id_t m_index_id;
    catalog::index_type_t m_index_type;
    index_key_schema_t m_key_schema;
    bool m_is_unique;

    // Recursive_mutex is used here because shared_mutex cannot be unlocked multiple times on the same thread.
    // This is a requirement because the implementation requires a reader to lock when obtaining the start
    // and end iterators. In future, the index resides in shared memory and should ideally be lock-free.
    mutable std::recursive_mutex m_index_lock;
};

} // namespace index
} // namespace db
} // namespace gaia
