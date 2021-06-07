/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <atomic>
#include <functional>
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
        message << "Cannot find index \"" << index_id << "\".";
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

struct index_record_t
{
    gaia::db::gaia_txn_id_t txn_id;
    gaia::db::gaia_offset_t offset;
    gaia::db::gaia_locator_t locator;
    uint8_t deleted;

    friend std::ostream& operator<<(std::ostream& os, const index_record_t& rec);
};

class base_index_t
{
public:
    base_index_t(gaia::common::gaia_id_t index_id, catalog::index_type_t index_type)
        : m_index_id(index_id), m_index_type(index_type)
    {
    }
    virtual ~base_index_t() = default;

    gaia::common::gaia_id_t id() const;
    catalog::index_type_t type() const;

    std::recursive_mutex& get_lock() const;

    bool operator==(const base_index_t& other) const;

    virtual void clear() = 0;
    virtual common::generator_t<index_record_t> generator(gaia_txn_id_t txn_id) = 0;

private:
    gaia::common::gaia_id_t m_index_id;
    catalog::index_type_t m_index_type;

protected:
    // Recursive_mutex is used here because shared_mutex cannot be unlocked multiple times on the same thread.
    // This is a requirement because the implementation requires a reader to lock when obtaining the start
    // and end iterators. In future, the index resides in shared memory and should ideally be lock-free.
    mutable std::recursive_mutex m_index_lock;
};

} // namespace index
} // namespace db
} // namespace gaia
