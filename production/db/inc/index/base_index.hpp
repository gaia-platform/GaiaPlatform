/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/db/db_types.hpp"

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

class invalid_index_type : public common::gaia_exception
{
public:
    explicit invalid_index_type()
    {
        std::stringstream message;
        message << "Invalid index type. ";
        m_message = message.str();
    }
};

enum class index_record_operation_t : uint8_t
{
    not_set,

    insert,
    remove,
    // Updates that modify the index key will issue remove and insert.
    // Updates that do not modify the index key will issue update_remove and update_insert.
    update_remove,
    update_insert,
};

struct index_record_t
{
    // The following fields should occupy 3x64bit.
    gaia::db::gaia_txn_id_t txn_id;
    gaia::db::gaia_locator_t locator;
    gaia::db::gaia_offset_t offset;
    index_record_operation_t operation;

    friend std::ostream& operator<<(std::ostream& os, const index_record_t& rec);
};

// We use this assert to check that the index record structure is packed optimally.
static_assert(sizeof(index_record_t) == 24, "index_record_t size has changed unexpectedly!");

class index_key_t;

class base_index_t
{
public:
    base_index_t(gaia::common::gaia_id_t index_id, catalog::index_type_t index_type, bool is_unique)
        : m_index_id(index_id), m_index_type(index_type), m_is_unique(is_unique)
    {
    }
    virtual ~base_index_t() = default;

    gaia::common::gaia_id_t id() const;
    catalog::index_type_t type() const;
    bool is_unique() const;

    std::recursive_mutex& get_lock() const;

    bool operator==(const base_index_t& other) const;

    virtual void clear() = 0;
    virtual std::shared_ptr<common::iterators::generator_t<index_record_t>> generator(gaia_txn_id_t txn_id) = 0;
    virtual std::shared_ptr<common::iterators::generator_t<index_record_t>> equal_range_generator(gaia_txn_id_t txn_id, const index_key_t& key) = 0;

protected:
    gaia::common::gaia_id_t m_index_id;
    catalog::index_type_t m_index_type;
    bool m_is_unique;

    // Recursive_mutex is used here because shared_mutex cannot be unlocked multiple times on the same thread.
    // This is a requirement because the implementation requires a reader to lock when obtaining the start
    // and end iterators. In future, the index resides in shared memory and should ideally be lock-free.
    mutable std::recursive_mutex m_index_lock;
};

} // namespace index
} // namespace db
} // namespace gaia
